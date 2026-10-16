# Process

A lightweight, header-only C++20 process management utility for Linux.

Designed primarily for integration and system testing. The library provides a simple RAII wrapper around `fork()`, `execv()`, `waitpid()`, and POSIX signals.

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
- C++20 compiler

## Potential Extensions

The current implementation intentionally focuses on the most common testing use cases.

Potential future additions:

- stdout redirection
- stderr redirection
- stdin redirection
- environment variable injection
- working directory selection
