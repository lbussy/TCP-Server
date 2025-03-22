/**
 * @file main.cpp
 * @brief Entry point for the TCP server application.
 * @details This file initializes the TCP server, handles signal-based shutdown,
 *          and ensures a clean exit upon termination.
 *
 * @author Lee C. Bussy (@lbussy)
 * @date 2025-02-10
 *
 * @note The server listens on localhost (`127.0.0.1`) for security reasons.
 * @note Graceful shutdown is handled using signal trapping.
 */

// Project includes
#include "tcp_server.hpp"
#include "tcp_command_handler.hpp"

// Standard includes
#include <atomic>
#include <csignal>
#include <iostream>
#include <thread>

// System Includes

/// @brief Port number for the TCP server.
#define SERVERPORT 31415

/// @brief Atomic flag to indicate whether the server is running.
/// @note Declared as `extern` in a header if shared across files.
std::atomic<bool> running(true); // Ensure this is defined ONLY in main.cpp

/// @brief Global pointers to allow handling for the server instance
TCP_Server server;
TCP_Commands handler;

/**
 * @brief Signal handler to gracefully stop the server.
 * @details Captures SIGINT and SIGTERM to ensure a clean shutdown.
 *
 * @param signal The signal number received (SIGINT, SIGTERM).
 */
void signalHandler(int signal)
{
    if (signal == SIGINT || signal == SIGTERM)
    {
        if (server.isRunning())
        {
            server.stop(); // Ensure clean shutdown on Ctrl+C
        }
    }
}

/**
 * @brief Entry point for the TCP server.
 * @details Initializes logging, creates a TCP server instance, and handles
 *          signal-based shutdown while keeping the server running.
 *
 * @return Returns `0` on successful execution, or `1` on failure.
 */
int main()
{
    //handler;
    server.start(SERVERPORT, handler);

    // Register signal handlers for graceful shutdown
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    // Start the server
    if (!server.isRunning())
    {
        return 1;
    }

    // Wait for server to stop before exiting
    while (server.isRunning())
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    std::cout  << "Exiting main." << std::endl;
    return 0;
}
