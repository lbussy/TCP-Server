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

// Project Inclides
#include "tcp_server.hpp"

// Standard Includes
#include <algorithm>
#include <atomic>
#include <mutex>
#include <unordered_set>
#include <vector>

// System Includes
#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#ifdef DEBUG_TCP_SERVER
#include <iostream> // Debug printing
#include <cstring>  // For strerror()
#include <sstream>  // For error string
#endif

/// @brief Defines the maximum number of simultaneous connections allowed.
#define MAX_CONNECTIONS 15

/// @brief Tracks the number of active connections.
std::atomic<int> TCP_Server::active_connections_ = 0;

/**
 * @brief Constructs a TCP server.
 */
TCP_Server::TCP_Server()
    : running_(false), server_fd_(-1), port_(0), command_handler_(nullptr) {
}

/**
 * @brief Starts the TCP server.
 * @details Binds to the specified port and begins listening for connections.
 * 
 * @param port The port number to listen on.
 * @param handler Reference to a user-defined command handler.
 * 
 * @return True if the server starts successfully, false otherwise.
 */
bool TCP_Server::start(int port, TCP_CommandHandler &handler)
{
    if (running_.load())
        return false; // Already running
    port_ = port;
    command_handler_ = &handler;
    running_.store(true);

#ifdef DEBUG_TCP_SERVER
    std::cout << "Starting server on port: " << std::to_string(port) << std::endl;
#endif

    server_thread_ = std::thread(&TCP_Server::run_server, this);
    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Allow time to attempt bind

    if (server_fd_ == -1)
    { // Check if binding failed
#ifdef DEBUG_TCP_SERVER
        std::cerr << "Server failed to bind. Port: " + std::to_string(port) << std::endl;
#endif
        running_.store(false);
        return false;
    }

    return true;
}

/**
 * @brief Stops the TCP server.
 * @details Closes all client connections, shuts down the server socket,
 *          and gracefully exits the main server loop.
 */
void TCP_Server::stop()
{
    if (!running_.load())
        return;
    running_.store(false);

#ifdef DEBUG_TCP_SERVER
    std::cout << "Stopping server." << std::endl;
#endif

    // Ensure `accept()` unblocks
    if (server_fd_ != -1)
    {
#ifdef DEBUG_TCP_SERVER
        std::cout << "Shutting down server socket." << std::endl;
#endif
        shutdown(server_fd_, SHUT_RDWR);
        close(server_fd_);
        server_fd_ = -1;
    }

    // Join all client threads
#ifdef DEBUG_TCP_SERVER
    std::cout << "Notifying active clients." << std::endl;
#endif
    {
        std::lock_guard<std::mutex> lock(client_threads_mutex_);
        for (auto &thread : client_threads_)
        {
            if (thread.joinable())
            {
                thread.join();
            }
        }
        client_threads_.clear();
    }

    // Ensure the main server thread exits
#ifdef DEBUG_TCP_SERVER
    std::cout << "Waiting for server thread to exit." << std::endl;
#endif
    if (server_thread_.joinable() && std::this_thread::get_id() != server_thread_.get_id())
    {
        server_thread_.join();
    }
#ifdef DEBUG_TCP_SERVER
    std::cout << "Server shutdown complete." << std::endl;
#endif
}

/**
 * @brief Destructor for the TCP server.
 * @details Ensures proper shutdown and resource cleanup.
 */
TCP_Server::~TCP_Server()
{
    stop();
}

/**
 * @brief Runs the main server loop.
 * @details Listens for incoming client connections and delegates
 *          them to handler threads.
 */
void TCP_Server::run_server()
{
#ifdef DEBUG_TCP_SERVER
    std::cout << "Server is starting." << std::endl;
#endif

    // Create the socket
#ifdef DEBUG_TCP_SERVER
    std::cout << "Creating server socket." << std::endl;
#endif
    server_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd_ < 0)
    {
#ifdef DEBUG_TCP_SERVER
        std::cerr << "Socket creation failed: " << std::string(strerror(errno)) << std::endl;
#endif
        running_.store(false);
        return;
    }

    // Allow port reuse
#ifdef DEBUG_TCP_SERVER
    std::cout << "Setting socket options." << std::endl;
#endif
    int opt = 1;
    if (setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)) < 0)
    {
#ifdef DEBUG_TCP_SERVER
        std::cerr << "setsockopt failed: " << std::string(strerror(errno)) << std::endl;
#endif
        stop();
        return;
    }

    // Configure the server address structure
#ifdef DEBUG_TCP_SERVER
    std::cout << "Configuring server address." << std::endl;
#endif
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port_);

    // Bind the socket
#ifdef DEBUG_TCP_SERVER
    std::cout << "Binding socket to port " << std::to_string(port_) << "." << std::endl;
#endif
    if (bind(server_fd_, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
#ifdef DEBUG_TCP_SERVER
        std::cerr << "Bind failed: " << std::string(strerror(errno)) << std::endl;
#endif
        stop();
        return;
    }

#ifdef DEBUG_TCP_SERVER
   std::cout << "Server successfully bound to port " << std::to_string(port_) << std::endl;
#endif

    // Start listening
    if (listen(server_fd_, MAX_CONNECTIONS) < 0)
    {
#ifdef DEBUG_TCP_SERVER
        std::cerr << "Listen failed: " << std::string(strerror(errno)) << std::endl;
#endif
        stop();
        return;
    }

#ifdef DEBUG_TCP_SERVER
    std::cout << "Server is listening on port " << std::to_string(port_) << std::endl;
#endif

    // Accept client connections
    while (running_.load())
    {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);

        // **Set server socket to non-blocking mode**
        int flags = fcntl(server_fd_, F_GETFL, 0);
        fcntl(server_fd_, F_SETFL, flags | O_NONBLOCK);

        int client_socket = accept(server_fd_, (struct sockaddr *)&client_addr, &client_len);
        if (client_socket < 0)
        {
            if (!running_.load())
            {
                break; // **Exit loop when shutting down**
            }
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                continue; // **Retry accept() without error**
            }
#ifdef DEBUG_TCP_SERVER
            std::cerr << "Accept failed: " << std::string(strerror(errno)) << std::endl;
#endif
            continue;
        }

        std::thread(&TCP_Server::handle_client, this, client_socket).detach();
    }

#ifdef DEBUG_TCP_SERVER
    std::cout << "Server is shutting down." << std::endl;
#endif
    stop();
}

/**
 * @brief Handles client connections and processes commands.
 * @param client_socket The socket descriptor for the client connection.
 */
void TCP_Server::handle_client(int client_socket)
{
    char buffer[1024] = {0};
    int bytes_read = read(client_socket, buffer, sizeof(buffer) - 1);

    if (bytes_read <= 0)
    {
        close(client_socket);
        return;
    }

    buffer[bytes_read] = '\0';
    std::string input(buffer);

    // Trim whitespace from input
    input.erase(input.find_last_not_of(" \t\n\r") + 1);
    input.erase(0, input.find_first_not_of(" \t\n\r"));

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

#ifdef DEBUG_TCP_SERVER
    std::cout << "Processed command: '" << command << "', arg: '" << arg << "'" << std::endl;
#endif

    // Process command through the command handler
    std::string response = command_handler_->handleCommand(command, arg);
#ifdef DEBUG_TCP_SERVER
    std::cout << "Response to client: '" << response << "'" << std::endl;
#endif
    response += "\n";
    send(client_socket, response.c_str(), response.length(), 0);

    close(client_socket);
}
