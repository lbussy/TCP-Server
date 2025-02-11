# TCP-Server Library

## Overview

The TCP-Server library is a lightweight, multi-threaded TCP server designed for handling structured command-based communication. It listens on `localhost` and delegates command processing to a user-defined class that implements a standardized interface.

## Features

- ✅ Multi-threaded client handling
- ✅ Customizable command processing (`tcp_command_handler.*`)
- ✅ Supports adding/removing commands dynamically
- ✅ Graceful shutdown with `SIGINT` and `SIGTERM` handling
- ✅ Built-in logging via `LCBLog`
- ✅ Test Python client (`scripts/tcp_server_test.py`) for command verification

---

## Installation & Compilation

### Cloning the Repository

Since this project uses `LCBLog` as a submodule, you must clone recursively:

``` bash
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

### Running the Server

Use the `test` target, which will compile and execute the test server.

``` bash
make test
```

By default, the server listens on port 31415.

### Stopping the Server

To stop the server, press:

``` bash
CTRL+C
```

or send a SIGTERM signal:

``` bash
kill -SIGTERM $(pgrep tcp_test)
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

This project uses `LCBLog` for structured logging.
Modify logging levels in `main.cpp`:

``` cpp
llog.setLogLevel(DEBUG);
```

Available levels:

- `DEBUG`
- `INFO`
- `WARN`
- `ERROR`
- `FATAL`

---

## Contributing

Pull requests are welcome! If you add new features, please update the documentation.

---

## License

This project is licensed under the MIT License.
