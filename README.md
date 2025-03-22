# TCP-Server Library

## Overview

The TCP-Server library is a lightweight, multi-threaded TCP server designed for handling structured, command-based communication. It listens on `localhost` and delegates command processing to a user-defined class that implements a standardized interface.

---

## Features

- ✅ **Multi-threaded client handling** – Each client connection is processed in its own thread.
- ✅ **Customizable command processing** – Customize commands using your own implementation in `tcp_command_handler.*`.
- ✅ **Dynamic command management** – Supports adding or removing commands on the fly.
- ✅ **Graceful shutdown** – Signal-based shutdown (`SIGINT`/`SIGTERM`) with condition variable support for clean exit.
- ✅ **Asynchronous logging** – Uses a dedicated logger thread (via `AsyncLogger`) to print full log messages without interleaving.
- ✅ **Callback with Priority Support** – Server events are reported via a callback that accepts a priority enum (DEBUG, INFO, WARN, ERROR, FATAL), a message, and a success flag.
- ✅ **Thread scheduling control** – Use `setPriority()` to adjust the server thread's scheduling policy and priority at runtime.
- ✅ **Test Python client** – A Python script (`scripts/tcp_server_test.py`) is provided for command verification.

---

## Installation & Compilation

### Cloning the Repository

Since this project uses `LCBLog` as a submodule, you must clone recursively:

```bash
git clone --recurse-submodules https://github.com/yourusername/TCP-Server.git
cd TCP-Server
```

If you’ve already cloned without submodules, initialize them manually:

``` bash
git submodule update --init --recursive
```

### Building the Server

Ensure you have a C++17-compatible compiler and make, then run:

``` bash
cd src
make
```

This will generate the `tcp_server` objects in the src directory.

---

### Running the Server

#### Starting the Server

Use the `test` target, which will compile and execute the test server.

``` bash
make test
```

The server is started from main.cpp with a callback that formats log messages in the form:

```text
[PRIORITY]: message
```

For example, in your code you can start the server like this:

```cpp
server.start(SERVERPORT, &handler,
    [](TCP_Server::Priority priority, const std::string &msg, bool success) {
        std::string priorityStr;
        switch(priority) {
            case TCP_Server::Priority::DEBUG: priorityStr = "DEBUG"; break;
            case TCP_Server::Priority::INFO:  priorityStr = "INFO "; break;
            case TCP_Server::Priority::WARN:  priorityStr = "WARN "; break;
            case TCP_Server::Priority::ERROR: priorityStr = "ERROR"; break;
            case TCP_Server::Priority::FATAL: priorityStr = "FATAL"; break;
            default: priorityStr = "UNKWN"; break;
        }
        std::string fullMsg = "[" + priorityStr + "]: " + msg;
        gLogger.log(fullMsg);
    }
);
```

#### Thread Scheduling

You can adjust the server thread’s scheduling policy and priority after startup using the setPriority() method. For example:

```cpp
if (!server.setPriority(SCHED_RR, 20)) {
    std::cerr << "Failed to update server thread priority." << std::endl;
}
```

### Stopping the Server

To stop the demo server, press:

``` bash
CTRL+C
```

or send a SIGTERM signal:

``` bash
kill -SIGTERM $(pgrep tcp_test)
```

In your own code, you would stop the server threads with:

```cpp
if (server.isRunning())
{
    server.stop(); // Ensure clean shutdown on Ctrl+C or SIGTERM.
}
```

---

## Testing with the Python Client

A test Python script is provided in the `scripts/` directory.
This script allows you to send commands to the TCP server interactively.

### Running the Python Client

Ensure Python 3 is installed, then execute:

``` bash
cd scripts
python3 tcp_server_test.py
```

The client will connect to `127.0.0.1:31415` and allow you to enter commands.

### Example Session

``` text
TCP Client connected to 127.0.0.1:31415 (type 'exit' to quit)
Enter command: call
Response: Call <example response>
Enter command: power 100
Response: Power set to 100
Enter command: unknown
Response: ERROR: Unknown command 'unknown'. Type 'help' for a list of commands.
Enter command: exit
```

---

## Customizing Command Handling

The file `tcp_command_handler.*` provides an example implementation of a command handler. Developers should modify this file to define their own commands.

---

## Modifying Commands

Each command has a dedicated method.
For example, the existing `handlePower()` method:

``` cpp
std::string TCP_Commands::handlePower(const std::string &arg) {
    return arg.empty() ? "Power <example response>" : "Power set to " + arg;
}
```

You can customize it:

``` c++
std::string TCP_Commands::handlePower(const std::string &arg) {
    if (arg.empty()) {
        return "Power is currently at 50W";  // Example default response
    } else {
        return "Power adjusted to " + arg + "W";
    }
}
```

### Adding New Commands

1. Declare a new handler method in `tcp_command_handler.hpp`:

    ``` cpp
    std::string handleMode(const std::string &arg);
    ```

2. Implement the method in `tcp_command_handler.cpp`:

    ``` cpp
    std::string TCP_Commands::handleMode(const std::string &arg) {
        return arg.empty() ? "Mode is AUTO" : "Mode set to " + arg;
    }
    ```

3. Register the command in `initializeHandlers()`:

    ``` cpp
    command_handlers["mode"] = [this](const std::string &arg) { return handleMode(arg); };
    ```

### Removing a Command

To remove a command, delete its handler function and remove it from `initializeHandlers()`.

---

## Logging

This project now uses an asynchronous logger (AsyncLogger) for structured logging. The logger prints complete messages (e.g., [DEBUG]: message) without interleaving, ensuring clarity when multiple messages are logged concurrently.  From within the class, use the callback to receive and print debug messages.

Available levels:

- `DEBUG`
- `INFO`
- `WARN`
- `ERROR`
- `FATAL`

e.g.:

```cpp
callback(Priority::INFO, "Server stopped.", true);
```

---

## Contributing

Pull requests are welcome! If you add new features, please update the documentation.

---

## License

This project is licensed under the MIT License.
