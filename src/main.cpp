/**
 * @file main.cpp
 * @brief Entry point for the TCP server application.
 * @details This file initializes the TCP server, handles signal-based shutdown,
 *          and ensures a clean exit upon termination.
 *
 * @copyright Copyright (c) 2025 Lee C. Bussy (@lbussy)
 * @license MIT License (see LICENSE file for details)
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
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <queue>
#include <string>
#include <thread>

/// @brief Port number for the TCP server.
constexpr const int SERVERPORT = 31415;

/// @brief Atomic flag to indicate whether the server is running.
std::atomic<bool> running(true);

/// @brief Global pointers to allow handling for the server instance
TCP_Server server;
TCP_Commands handler;

// Global condition variable and mutex for waiting until the server stops.
std::mutex cv_mutex;
std::condition_variable cv;

// A simple asynchronous logger.
class AsyncLogger
{
public:
    AsyncLogger() : stopFlag(false)
    {
        workerThread = std::thread(&AsyncLogger::worker, this);
    }

    ~AsyncLogger()
    {
        {
            std::lock_guard<std::mutex> lock(queueMutex);
            stopFlag = true;
        }
        cv.notify_all();
        if (workerThread.joinable())
        {
            workerThread.join();
        }
    }

    // Non-blocking log; caller just enqueues the message.
    void log(const std::string &msg)
    {
        {
            std::lock_guard<std::mutex> lock(queueMutex);
            messageQueue.push(msg);
        }
        cv.notify_one();
    }

private:
    std::queue<std::string> messageQueue;
    std::mutex queueMutex;
    std::condition_variable cv;
    bool stopFlag;
    std::thread workerThread;

    // The worker thread prints messages from the queue.
    void worker()
    {
        while (true)
        {
            std::string msg;
            {
                std::unique_lock<std::mutex> lock(queueMutex);
                cv.wait(lock, [this]
                        { return !messageQueue.empty() || stopFlag; });
                if (stopFlag && messageQueue.empty())
                    break;
                msg = std::move(messageQueue.front());
                messageQueue.pop();
            }
            // Print the whole message atomically.
            std::cout << msg << std::endl;
        }
    }
};

// Async logger for testing
AsyncLogger gLogger;

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
            server.stop(); // Ensure clean shutdown on Ctrl+C or SIGTERM.
        }
        // Notify the condition variable so main() can wake up.
        cv.notify_all();
    }
}

/**
 * @brief Callback function for the TCP server.
 *
 * Converts the provided Priority enum into a string and enqueues the full message
 * for asynchronous printing.
 *
 * @param priority The priority level.
 * @param msg The message to print.
 * @param success Whether the operation succeeded.
 */
void callback_tcp_server(TCP_Server::Priority priority, const std::string &msg, bool success)
{
    std::string priorityStr;
    switch (priority)
    {
    case TCP_Server::Priority::DEBUG:
        priorityStr = "DEBUG";
        break;
    case TCP_Server::Priority::INFO:
        priorityStr = "INFO ";
        break;
    case TCP_Server::Priority::WARN:
        priorityStr = "WARN ";
        break;
    case TCP_Server::Priority::ERROR:
        priorityStr = "ERROR";
        break;
    case TCP_Server::Priority::FATAL:
        priorityStr = "FATAL";
        break;
    default:
        priorityStr = "UNKWN";
        break;
    }
    std::string fullMsg = "[" + priorityStr + "] TCPSERVER: " + msg;
    gLogger.log(fullMsg);
}

/**
 * @brief Entry point for the TCP server.
 * @details Initializes logging, creates a TCP server instance, and handles
 *          signal-based shutdown while waiting on a condition variable.
 *
 * @return Returns 0 on successful execution, or 1 on failure.
 */
int main()
{
    // Register signal handlers for graceful shutdown.
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    // Start the TCP server with our callback.
    // server.start(SERVERPORT, &handler);
    server.start(SERVERPORT, &handler, callback_tcp_server);
    server.setPriority(SCHED_RR, 10);

    // Wait for the server to stop using a condition variable.
    std::unique_lock<std::mutex> lock(cv_mutex);
    cv.wait(lock, []
            { return !server.isRunning(); });

    gLogger.log("Exiting main.");
    return 0;
}
