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

#include "tcp_server.hpp"
#include "tcp_command_handler.hpp"

#include <iostream>
#include <csignal>
#include <thread>
#include <atomic>

/// @brief Port number for the TCP server.
#define SERVERPORT 31415

/// @brief Atomic flag to indicate whether the server is running.
/// @note Declared as `extern` in a header if shared across files.
std::atomic<bool> running(true); // Ensure this is defined ONLY in main.cpp

/// @brief Global pointer to allow signal handling for the server instance.
TCP_Server *serverInstance = nullptr;

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
        if (serverInstance)
        {
            serverInstance->stop(); // Ensure clean shutdown on Ctrl+C
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
    LCBLog llog;
    llog.setLogLevel(DEBUG);

    TCP_Commands handler;
    TCP_Server server(SERVERPORT, llog, handler);
    serverInstance = &server;

    // Register signal handlers for graceful shutdown
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    // Start the server
    if (!server.start())
    {
        return 1;
    }

    // Wait for server to stop before exiting
    while (serverInstance->isRunning())
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    llog.logS(INFO, "Exiting main.");
    return 0;
}
