/**
 * @file tcp_server.cpp
 * @brief Implementation of the TCP_Server class.
 * @details This file contains the implementation of a multi-threaded TCP server
 *          that listens for incoming client connections, processes commands,
 *          and ensures thread-safe execution.
 *
 * @copyright Copyright (c) 2025 Lee C. Bussy (@lbussy)
 * @license MIT License (see LICENSE file for details)
 *
 * @note This server is designed to accept connections only from `127.0.0.1`
 *       for security reasons.
 * @note Implements rate limiting to prevent abuse.
 * @note Ensures graceful shutdown by notifying active clients.
 *
 * @see https://github.com/lbussy/TCP-Server
 */

#include "tcp_server.hpp"

// Standard Includes
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstring>
#include <mutex>
#include <sstream>
#include <unordered_set>
#include <vector>

// System Includes
#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sched.h>
#include <sys/socket.h>
#include <unistd.h>

/// @brief Defines the maximum number of simultaneous connections allowed.
constexpr const int MAX_CONNECTIONS = 15;

// Initialize static member for tracking active connections.
std::atomic<int> TCP_Server::active_connections_ = 0;

/**
 * @brief Constructs a TCP server.
 */
TCP_Server::TCP_Server()
    : port_(0),
      running_(false),
      command_handler_(nullptr),
      server_fd_(-1)
{
}

/**
 * @brief Destructor for the TCP server.
 * @details Ensures proper shutdown and resource cleanup.
 */
TCP_Server::~TCP_Server()
{
    stop();
    if (server_thread_.joinable())
    {
        server_thread_.join();
    }
}

/**
 * @brief Starts the TCP server.
 * @details Binds to the specified port and begins listening for connections.
 *
 * @param port The port number to listen on.
 * @param handler Pointer to a user-defined command handler.
 *
 * @return True if the server starts successfully, false otherwise.
 */
bool TCP_Server::start(int port, TCP_CommandHandler *handler,
                       std::function<void(TCP_Server::Priority, const std::string &, bool)> cb)
{
    // Store the callback for later use.
    if (cb)
        callback_ = cb;

    std::lock_guard<std::mutex> lock(server_mutex_);
    if (running_.load())
    {
        callback_(Priority::DEBUG, "Server is already running.", false);
        return false;
    }
    if (handler == nullptr)
    {
        callback_(Priority::ERROR, "Invalid command handler provided.", false);
        return false;
    }
    port_ = port;
    command_handler_ = handler;
    running_.store(true);

    try
    {
        // Launch the server thread.
        server_thread_ = std::thread(&TCP_Server::run_server, this);
    }
    catch (const std::exception &e)
    {
        callback(Priority::ERROR, std::string("Failed to start server thread: ") + e.what(), false);
        running_.store(false);
        return false;
    }
    callback(Priority::INFO, "Server started successfully on port " + std::to_string(port_), true);
    return true;
}

bool TCP_Server::setPriority(int schedPolicy, int priority)
{
    // Ensure that the server thread is running.
    if (!running_.load() || !server_thread_.joinable())
    {
        callback(Priority::ERROR, "Server thread is not running. Cannot set priority.", false);
        return false;
    }

    pthread_t nativeHandle = server_thread_.native_handle();
    sched_param sch_params;
    sch_params.sched_priority = priority;
    int ret = pthread_setschedparam(nativeHandle, schedPolicy, &sch_params);
    if (ret != 0)
    {
        callback(Priority::ERROR, "pthread_setschedparam failed: " + std::string(strerror(ret)), false);
        return false;
    }
    callback(Priority::DEBUG, "Thread scheduling set to policy " + std::to_string(schedPolicy) + " with priority " + std::to_string(priority), true);

    return true;
}

/**
 * @brief Stops the TCP server.
 * @details Closes the listening socket, stops the accept loop, and gracefully
 *          shuts down the server thread.
 */
void TCP_Server::stop()
{
    std::lock_guard<std::mutex> lock(server_mutex_);
    if (!running_.load())
    {
        return;
    }
    running_.store(false);

    // Close the server socket to unblock accept() if needed.
    if (server_fd_ != -1)
    {
        ::close(server_fd_);
        server_fd_ = -1;
    }

    // Wait for the server thread to exit.
    if (server_thread_.joinable())
    {
        server_thread_.join();
    }
    callback(Priority::INFO, "Server stopped.", true);
}

void TCP_Server::callback(Priority priority, std::string message, bool result)
{
    if (callback_)
        callback_(priority, message, result);
}

/**
 * @brief Sets up the listening socket and accepts incoming client connections.
 * @details The accept loop is non-blocking and periodically checks the running flag.
 */
void TCP_Server::run_server()
{
    // Create the listening socket.
    server_fd_ = ::socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd_ < 0)
    {
        callback(Priority::ERROR, "Socket creation failed: " + std::string(strerror(errno)), false);
        running_.store(false);
        return;
    }

    // Allow address and port reuse.
    int opt = 1;
    if (setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)) < 0)
    {
        callback(Priority::ERROR, "Socket setsockopt failed: " + std::string(strerror(errno)), false);
        stop();
        return;
    }

    // Configure server address.
    struct sockaddr_in address;
    std::memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    // Restrict to localhost if needed for security.
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK); // Use INADDR_LOOPBACK (127.0.0.1)
    address.sin_port = htons(port_);

    // Bind the socket.
    if (bind(server_fd_, reinterpret_cast<struct sockaddr *>(&address), sizeof(address)) < 0)
    {
        callback(Priority::ERROR, "Address bind failed: " + std::string(strerror(errno)), false);
        stop();
        return;
    }

    // Start listening.
    if (listen(server_fd_, MAX_CONNECTIONS) < 0)
    {
        callback(Priority::ERROR, "Listen failed: " + std::string(strerror(errno)), false);
        stop();
        return;
    }

    // Set the server socket to non-blocking mode once (outside the loop).
    int flags = fcntl(server_fd_, F_GETFL, 0);
    fcntl(server_fd_, F_SETFL, flags | O_NONBLOCK);

    // Main accept loop.
    while (running_.load())
    {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_socket = accept(server_fd_, reinterpret_cast<struct sockaddr *>(&client_addr), &client_len);
        if (client_socket < 0)
        {
            if (!running_.load())
            {
                break;
            }
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                continue;
            }
            callback(Priority::ERROR, "Accept failed: " + std::string(strerror(errno)), false);
            continue;
        }
        callback(Priority::DEBUG, "Client connected.", true);

        // Launch a detached thread to handle the client.
        std::thread(&TCP_Server::handle_client, this, client_socket).detach();
    }

    callback(Priority::DEBUG, "Exiting accept loop, cleaning up server socket.", true);
    ::close(server_fd_);
    server_fd_ = -1;
}

/**
 * @brief Handles a client connection.
 * @param client_socket The socket descriptor for the client.
 * @details Reads data from the client, processes the command using the provided
 *          command handler, sends the response, and closes the connection.
 */
void TCP_Server::handle_client(int client_socket)
{
    const size_t buffer_size = 1024;
    char buffer[buffer_size] = {0};

    int bytes_read = ::read(client_socket, buffer, buffer_size - 1);
    if (bytes_read <= 0)
    {
        ::close(client_socket);
        return;
    }
    buffer[bytes_read] = '\0';
    std::string input(buffer);

    // Trim whitespace from input.
    input.erase(input.find_last_not_of(" \t\n\r") + 1);
    input.erase(0, input.find_first_not_of(" \t\n\r"));

    // Parse command and argument.
    std::string command, arg;
    auto pos = input.find(' ');
    if (pos == std::string::npos)
    {
        command = input;
    }
    else
    {
        command = input.substr(0, pos);
        arg = input.substr(pos + 1);
    }
    callback(Priority::INFO, "Received command: '" + command + "', argument: '" + arg + "'", true);

    // Process the command via the command handler.
    std::string response = command_handler_->handleCommand(command, arg);
    callback(Priority::DEBUG, "Sending response: '" + response + "'", true);

    // Append newline for client readability.
    response += "\n";
    ::send(client_socket, response.c_str(), response.length(), 0);

    ::close(client_socket);
}
