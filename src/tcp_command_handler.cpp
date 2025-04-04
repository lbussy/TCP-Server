/**
 * @file tcp_command_handler.cpp
 * @brief Example implementation of a TCP command handler.
 * @details This file provides a structured example of command handling
 *          for use with the TCP-Server library. It demonstrates how to
 *          implement a custom command handler by assigning a dedicated
 *          method to each supported command.
 *
 * @copyright Copyright (c) 2025 Lee C. Bussy (@lbussy)
 * @license MIT License (see LICENSE file for details)
 *
 * @note This file is included as a functional example in the TCP-Server
 *       library. Developers integrating TCP-Server into their projects
 *       should extend or modify this handler to fit their specific
 *       command requirements.
 *
 * @see https://github.com/lbussy/TCP-Server
 */

#include "tcp_command_handler.hpp"

/**
 * @brief Constructs the TCP_Commands.
 * @details Initializes the list of valid commands and maps them to handlers.
 */
TCP_Commands::TCP_Commands()
{
    valid_commands = {
        "transmit", "call", "grid", "power", "freq", "ppm", "selfcal",
        "offset", "led", "port", "xmit", "version", "help"};

    // Initialize command handlers
    initializeHandlers();
}

/**
 * @brief Initializes the mapping of commands to their corresponding handlers.
 */
void TCP_Commands::initializeHandlers()
{
    // Handlers requiring an argument:
    command_handlers["transmit"] = [this](const std::string &arg)
    { return handleTransmit(arg); };
    command_handlers["call"] = [this](const std::string &arg)
    { return handleCall(arg); };
    command_handlers["grid"] = [this](const std::string &arg)
    { return handleGrid(arg); };
    command_handlers["power"] = [this](const std::string &arg)
    { return handlePower(arg); };
    command_handlers["freq"] = [this](const std::string &arg)
    { return handleFreq(arg); };
    command_handlers["ppm"] = [this](const std::string &arg)
    { return handlePPM(arg); };
    command_handlers["selfcal"] = [this](const std::string &arg)
    { return handleSelfCal(arg); };
    command_handlers["offset"] = [this](const std::string &arg)
    { return handleOffset(arg); };
    command_handlers["led"] = [this](const std::string &arg)
    { return handleLED(arg); };

    // Handlers that do not require an argument:
    command_handlers["port"] = [this](const std::string &)
    { return handlePort(); };
    command_handlers["xmit"] = [this](const std::string &)
    { return handleXmit(); };
    command_handlers["version"] = [this](const std::string &)
    { return handleVersion(); };
    command_handlers["help"] = [this](const std::string &)
    { return handleHelp(); };
}

/**
 * @brief Processes a command by calling the appropriate handler function.
 * @param command The command name.
 * @param arg The argument passed with the command.
 * @return A response string based on the command.
 */
std::string TCP_Commands::processCommand(const std::string &command, const std::string &arg)
{
    auto it = command_handlers.find(command);
    if (it != command_handlers.end())
    {
        return it->second(arg);
    }
    return "ERROR: Unknown command '" + command + "'. Type 'help' for a list of commands.";
}

/**
 * @brief Handles an incoming command.
 * @details Determines if the command is valid and routes it to the appropriate handler.
 * @param command The command name.
 * @param arg The argument (if any).
 * @return Response string generated by the handler.
 */
std::string TCP_Commands::handleCommand(const std::string &command, const std::string &arg)
{
    return processCommand(command, arg);
}

/**
 * @brief Retrieves the list of valid commands.
 * @return A set containing valid command strings.
 */
const std::unordered_set<std::string> &TCP_Commands::getValidCommands() const
{
    return valid_commands;
}

/**
 * @name Command Handlers
 * @brief Functions responsible for handling each command.
 * @details If an argument is provided, it parrots it back. Otherwise, it returns a default response.
 */
///@{

/// @brief Handles the "transmit" command.
std::string TCP_Commands::handleTransmit(const std::string &arg)
{
    return arg.empty() ? "Transmit <example response>" : "Transmit set to " + arg;
}

/// @brief Handles the "call" command.
std::string TCP_Commands::handleCall(const std::string &arg)
{
    return arg.empty() ? "Call <example response>" : "Call set to " + arg;
}

/// @brief Handles the "grid" command.
std::string TCP_Commands::handleGrid(const std::string &arg)
{
    return arg.empty() ? "Grid <example response>" : "Grid set to " + arg;
}

/// @brief Handles the "power" command.
std::string TCP_Commands::handlePower(const std::string &arg)
{
    return arg.empty() ? "Power <example response>" : "Power set to " + arg;
}

/// @brief Handles the "freq" command.
std::string TCP_Commands::handleFreq(const std::string &arg)
{
    return arg.empty() ? "Freq <example response>" : "Freq set to " + arg;
}

/// @brief Handles the "ppm" command.
std::string TCP_Commands::handlePPM(const std::string &arg)
{
    return arg.empty() ? "PPM <example response>" : "PPM set to " + arg;
}

/// @brief Handles the "selfcal" command.
std::string TCP_Commands::handleSelfCal(const std::string &arg)
{
    return arg.empty() ? "SelfCal <example response>" : "SelfCal set to " + arg;
}

/// @brief Handles the "offset" command.
std::string TCP_Commands::handleOffset(const std::string &arg)
{
    return arg.empty() ? "Offset <example response>" : "Offset set to " + arg;
}

/// @brief Handles the "led" command.
std::string TCP_Commands::handleLED(const std::string &arg)
{
    return arg.empty() ? "LED <example response>" : "LED set to " + arg;
}

/// @brief Handles the "port" command (no argument required).
std::string TCP_Commands::handlePort()
{
    return "Port <example response>";
}

/// @brief Handles the "xmit" command (no argument required).
std::string TCP_Commands::handleXmit()
{
    return "Xmit <example response>";
}

/// @brief Handles the "version" command (no argument required).
std::string TCP_Commands::handleVersion()
{
    return "Version 1.0.0";
}

/// @brief Handles the "help" command (no argument required).
std::string TCP_Commands::handleHelp()
{
    return "Available commands: transmit, call, grid, power, freq, ppm, selfcal, offset, led, port, xmit, version, help";
}
///@}
