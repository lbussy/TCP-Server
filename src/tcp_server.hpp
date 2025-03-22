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

#ifndef TCP_SERVER_HPP
#define TCP_SERVER_HPP

// Project includes
#include "tcp_command_handler.hpp" // Use an external command handler

// Standard includes
#include <atomic>
#include <functional>
#include <mutex>
#include <thread>

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
    // Public enum for callback priorities. Matches those in LCBLog.
    enum class Priority
    {
        DEBUG = 0, ///< Debug-level messages for detailed troubleshooting.
        INFO,      ///< Informational messages for general system state.
        WARN,      ///< Warnings indicating potential issues.
        ERROR,     ///< Errors that require attention but allow continued execution.
        FATAL      ///< Critical errors that result in program termination.
    };

    /**
     * @brief Constructs a TCP server instance.
     */
    TCP_Server();

    /**
     * @brief Destructor for the TCP server.
     * @details Ensures proper shutdown and resource cleanup.
     */
    ~TCP_Server();

    // Disable copying.
    TCP_Server(const TCP_Server &) = delete;
    TCP_Server &operator=(const TCP_Server &) = delete;

    /**
     * @brief Starts the TCP server.
     * @details Binds to the specified port and begins listening for connections.
     *
     * @param port The port number to listen on.
     * @param handler Pointer to a user-defined command handler.
     * @param callback Optional callback that will be invoked with a message string and a success flag.
     *                 For example: [](Priority::INFO, const std::string &msg, bool success){ ... }
     * @return True if the server starts successfully, false otherwise.
     */
    bool start(int port, TCP_CommandHandler *handler,
               std::function<void(TCP_Server::Priority, const std::string &, bool)> cb);

    /**
     * @brief Stops the TCP server.
     * @details Closes the listening socket and gracefully exits the main server loop.
     */
    void stop();

    /**
     * @brief Changes the scheduling policy and priority of the server thread.
     *
     * @param schedPolicy Scheduling policy to set (e.g., SCHED_FIFO, SCHED_RR).
     * @param priority The new thread priority.
     * @return True if the priority was successfully set; false otherwise.
     */
    bool setPriority(int schedPolicy, int priority);

    /**
     * @brief Checks if the server is currently running.
     * @return True if the server is running, false otherwise.
     */
    bool isRunning() const { return running_.load(); }

private:
    /// @brief Mutex for synchronizing server start/stop operations.
    std::mutex server_mutex_;

    /// @brief Main server thread responsible for listening for connections.
    std::thread server_thread_;

    /// @brief The port number on which the server is listening.
    int port_;

    /// @brief Indicates if the server is running.
    std::atomic<bool> running_;

    /// @brief Pointer to the command handler instance.
    TCP_CommandHandler *command_handler_;

    /// @brief The file descriptor for the server socket.
    int server_fd_;

    /// @brief Tracks the number of active connections.
    static std::atomic<int> active_connections_;

    // Store the callback so you can call it later from any method.
    std::function<void(Priority, const std::string &, bool)> callback_;

    void callback(Priority priority, std::string message, bool result);

    /**
     * @brief Runs the main server loop.
     * @details Listens for incoming client connections and delegates them
     *          to handler threads.
     */
    void run_server();

    /**
     * @brief Handles a client connection.
     * @param client_socket The socket descriptor for the client connection.
     * @details Reads input from the client, processes commands via the command handler,
     *          sends responses, and closes the connection.
     */
    void handle_client(int client_socket);
};

#endif // TCP_SERVER_HPP