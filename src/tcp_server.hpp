/**
 * @file tcp_server.hpp
 * @brief Header file for the TCP_Server class.
 * @details This file defines the interface for a multi-threaded TCP server
 *          that listens for incoming client connections, processes commands,
 *          and delegates command handling to a user-defined handler class.
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

#ifndef TCP_SERVER_H
#define TCP_SERVER_H

#include "tcp_command_handler.hpp" // Use an external command handler

#include <cstdint>
#include <cstring>
#include <cctype>
#include <atomic>
#include <mutex>
#include <vector>
#include <thread>
#include <unordered_map>
#include <chrono>

/**
 * @class TCP_Server
 * @brief A multi-threaded TCP server that processes user-defined commands.
 * @details This server listens for incoming TCP connections, spawns worker
 *          threads to handle clients, and forwards commands to a command handler.
 *          It ensures thread-safe operation and graceful shutdown.
 */
class TCP_Server
{
public:
    /**
     * @brief Constructs a TCP server instance.
     */
    TCP_Server();

    /**
     * @brief Destructor for the TCP server.
     * @details Ensures proper shutdown and resource cleanup.
     */
    ~TCP_Server();

    /**
     * @brief Starts the TCP server.
     * @details Binds to the specified port and begins listening for connections.
     * @return True if the server starts successfully, false otherwise.
     * 
     * @param port The port number to listen on.
     * @param handler Reference to a user-defined command handler.
     */
    bool start(int port, TCP_CommandHandler &handler);

    /**
     * @brief Stops the TCP server.
     * @details Closes all client connections, shuts down the server socket,
     *          and gracefully exits the main server loop.
     */
    void stop();

    /**
     * @brief Checks if the server is currently running.
     * @details This function provides a thread-safe way to determine whether
     *          the server is actively listening for connections.
     *
     * @return `true` if the server is running, `false` otherwise.
     */
    bool isRunning() const { return running_.load(); }

private:
    /**
     * @brief Indicates if the server is running.
     * @details This atomic flag is used to safely start and stop the server.
     */
    std::atomic<bool> running_;

    /**
     * @brief Mutex for synchronizing access to the server socket.
     */
    std::mutex server_mutex_;

    /**
     * @brief Stores client-handling threads.
     */
    std::vector<std::thread> client_threads_;

    /**
     * @brief Main server thread responsible for listening for connections.
     */
    std::thread server_thread_;

    /**
     * @brief Mutex for synchronizing client thread management.
     */
    std::mutex client_threads_mutex_;

    /**
     * @brief The file descriptor for the server socket.
     */
    int server_fd_;

    /**
     * @brief The port number on which the server is listening.
     */
    int port_;

    /**
     * @brief Reference to the command handler instance.
     * @details This is a user-implemented class that processes commands.
     */
    TCP_CommandHandler *command_handler_;

    /**
     * @brief Tracks the number of active connections.
     */
    static std::atomic<int> active_connections_;

    /**
     * @brief Runs the main server loop.
     * @details Listens for incoming client connections and delegates
     *          them to handler threads.
     */
    void run_server();

    /**
     * @brief Handles a client connection.
     * @details Reads input from the client, processes commands, and
     *          sends responses.
     * @param client_socket The socket descriptor for the client connection.
     */
    void handle_client(int client_socket);
};

#endif // TCP_SERVER_H
