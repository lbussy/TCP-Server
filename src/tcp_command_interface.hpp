/**
 * @file tcp_command_interface.hpp
 * @brief Abstract base class for handling TCP server commands.
 * @details This file defines the interface for implementing command handlers
 *          in the TCP-Server library. Derived classes must implement methods
 *          to process and handle incoming TCP commands.
 *
 * @copyright Copyright (c) 2025 Lee C. Bussy (@lbussy)
 * @license MIT License (see LICENSE file for details)
 *
 * @note This interface must be implemented by a custom handler to define
 *       specific command-processing logic.
 *
 * @see https://github.com/lbussy/TCP-Server
 */

#ifndef TCP_COMMAND_INTERFACE_H
#define TCP_COMMAND_INTERFACE_H

// Standard includes
#include <string>
#include <unordered_set>

/**
 * @class TCP_CommandHandler
 * @brief Abstract base class for handling TCP server commands.
 * @details This class defines the interface for command processing.
 *          Implementations must provide definitions for handling,
 *          processing, and retrieving valid commands.
 */
class TCP_CommandHandler
{
public:
    /**
     * @brief Handles a command received from the client.
     * @details This function is responsible for executing the appropriate
     *          response to a given command and its optional argument.
     *          Derived classes must implement this function.
     *
     * @param command The command string received from the client.
     * @param arg The argument associated with the command, if any.
     * @return A response string to be sent back to the client.
     */
    virtual std::string handleCommand(const std::string &command, const std::string &arg) = 0;

    /**
     * @brief Processes a command before handling.
     * @details Allows for command preprocessing, validation, or logging
     *          before execution.
     *
     * @param command The command string received from the client.
     * @param arg The argument associated with the command, if any.
     * @return A response string based on the processed command.
     */
    virtual std::string processCommand(const std::string &command, const std::string &arg) = 0;

    /**
     * @brief Retrieves the list of valid commands.
     * @details Provides a set of all recognized commands for reference.
     *          Implementations must define and populate this list.
     *
     * @return A set containing valid command strings.
     */
    virtual const std::unordered_set<std::string> &getValidCommands() const = 0;

    /**
     * @brief Virtual destructor for safe polymorphic deletion.
     * @details Defined inline to eliminate the need for a separate .cpp file.
     */
    virtual ~TCP_CommandHandler() = default;
};

#endif // TCP_COMMAND_INTERFACE_H
