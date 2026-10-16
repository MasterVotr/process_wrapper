# Process

A lightweight, header-only process management utility for Linux.

Designed primarily for integration and system testing. The library provides a simple RAII wrapper around `fork()`, `execv()`, `waitpid()`, and POSIX signals.

## Licence

This project is licensed under the MIT License.

See LICENCE file for details.

## Features

- Header-only
- Start external executables
- Pass command-line arguments
- Detect startup failures
- Query process state
- Wait for completion
- Timeout support
- SIGTERM, SIGINT and SIGKILL support
- Automatic cleanup

## Dependencies

- POSIX process APIs
- C++17 compiler
- Catch2 (for tests)

## Potential Extensions

The current implementation intentionally focuses on the most common testing use cases.

Potential future additions:

- stdout redirection
- stderr redirection
- stdin redirection
- environment variable injection
- working directory selection
