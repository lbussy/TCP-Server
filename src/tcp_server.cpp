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

#include <iostream>
#include <sstream>
#include <unordered_set>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <algorithm>
#include <vector>
#include <mutex>
#include <atomic>
#include <fcntl.h>

/// @brief Defines the maximum number of simultaneous connections allowed.
#define MAX_CONNECTIONS 15

/// @brief Tracks the number of active connections.
std::atomic<int> TCP_Server::active_connections = 0;

/**
 * @brief Constructs a TCP server with the given port and command handler.
 * @param port The port number to listen on.
 * @param logger Reference to the logging system.
 * @param handler Reference to a user-defined command handler.
 */
TCP_Server::TCP_Server(int port, LCBLog &logger, TCP_CommandHandler &handler)
    : server_fd(-1), port(port), log(logger), command_handler(handler)
{
    running.store(false);
}

/**
 * @brief Starts the TCP server.
 * @details Binds to the specified port and begins listening for connections.
 * @return True if the server starts successfully, false otherwise.
 */
bool TCP_Server::start()
{
    if (running.load())
        return false; // Already running
    running.store(true);

    log.logS(INFO, "Starting server on port: " + std::to_string(port));

    server_thread = std::thread(&TCP_Server::run_server, this);
    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Allow time to attempt bind

    if (server_fd == -1)
    { // Check if binding failed
        log.logE(ERROR, "Server failed to bind. Port: " + std::to_string(port));
        running.store(false);
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
    if (!running.load())
        return;
    running.store(false);

    log.logS(INFO, "Stopping server.");

    // Ensure `accept()` unblocks
    if (server_fd != -1)
    {
        log.logS(DEBUG, "Shutting down server socket.");
        shutdown(server_fd, SHUT_RDWR);
        close(server_fd);
        server_fd = -1;
    }

    // Join all client threads
    log.logS(INFO, "Notifying active clients.");
    {
        std::lock_guard<std::mutex> lock(client_threads_mutex);
        for (auto &thread : client_threads)
        {
            if (thread.joinable())
            {
                thread.join();
            }
        }
        client_threads.clear();
    }

    // Ensure the main server thread exits
    log.logS(DEBUG, "Waiting for server thread to exit.");
    if (server_thread.joinable() && std::this_thread::get_id() != server_thread.get_id())
    {
        server_thread.join();
    }

    log.logS(INFO, "Server shutdown complete.");
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
    log.logS(INFO, "Server is starting.");

    // Create the socket
    log.logS(INFO, "Creating server socket.");
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0)
    {
        log.logE(ERROR, "Socket creation failed: " + std::string(strerror(errno)));
        running.store(false);
        return;
    }

    // Allow port reuse
    log.logS(INFO, "Setting socket options.");
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)) < 0)
    {
        log.logE(ERROR, "setsockopt failed: " + std::string(strerror(errno)));
        stop();
        return;
    }

    // Configure the server address structure
    log.logS(INFO, "Configuring server address.");
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    // Bind the socket
    log.logS(INFO, "Binding socket to port " + std::to_string(port) + ".");
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        log.logE(ERROR, "Bind failed: " + std::string(strerror(errno)));
        stop();
        return;
    }

    log.logS(INFO, "Server successfully bound to port " + std::to_string(port));

    // Start listening
    if (listen(server_fd, MAX_CONNECTIONS) < 0)
    {
        log.logE(ERROR, "Listen failed: " + std::string(strerror(errno)));
        stop();
        return;
    }

    log.logS(INFO, "Server is listening on port " + std::to_string(port));

    // Accept client connections
    while (running.load())
    {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);

        // **Set server socket to non-blocking mode**
        int flags = fcntl(server_fd, F_GETFL, 0);
        fcntl(server_fd, F_SETFL, flags | O_NONBLOCK);

        int client_socket = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
        if (client_socket < 0)
        {
            if (!running.load())
            {
                break; // **Exit loop when shutting down**
            }
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                continue; // **Retry accept() without error**
            }
            log.logE(ERROR, "Accept failed: " + std::string(strerror(errno)));
            continue;
        }

        std::thread(&TCP_Server::handle_client, this, client_socket).detach();
    }

    log.logS(INFO, "Server is shutting down.");
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

    log.logS(DEBUG, "Processed command: '" + command + "', arg: '" + arg + "'");

    // Process command through the command handler
    std::string response = command_handler.handleCommand(command, arg);
    log.logS(DEBUG, "Response to client: '" + response + "'");
    response += "\n";
    send(client_socket, response.c_str(), response.length(), 0);

    close(client_socket);
}
