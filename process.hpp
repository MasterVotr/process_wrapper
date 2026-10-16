#pragma once

#include <array>
#include <cassert>
#include <cerrno>
#include <chrono>     // milliseconds
#include <csignal>    // kill
#include <cstdint>    // uint8_t
#include <ctime>      // pid_t
#include <fcntl.h>    // O_CLOEXEC
#include <filesystem> // path
#include <optional>
#include <string>
#include <sys/types.h> // pid_t
#include <sys/wait.h>  // waitpid
#include <thread>
#include <unistd.h> // fork, pipe2
#include <utility>
#include <vector>

namespace process
{

/**
 * @brief Lightweight process management utility for testing and integration
 * workflows that need external processes running (such as servers).
 *
 * Process provides a simple RAII wrapper around POSIX process creation and
 * handling (`fork`, `execv`, `waitpid`, `kill`). It is primarily intended for
 * testing requiring an external executable to be launched, monitored and
 * terminated.
 *
 * Typical example would be a message broker server that is needed for
 * intergration tests of message services.
 *
 * Process object is a unique owner of a child process and cannot be moved or
 * copied.
 */
class Process
{
  public:
    /**
     * @brief Current lifecycle state of the process.
     */
    enum class State : uint8_t
    {
        NotStarted, //* Process has not been started.
        Running,    //* Process is running.
        Failed,     //* Process has failed to start or an unexpected error occured.
        Exited,     //* Process has terminated. Its exit status is available.
    };

    /**
     * @brief Result of a completed process execution
     *
     * If exitNormally is true, code contains the process exit code returned
     * by main() or exit().
     *
     * If exitNormally if false, code contains the terminating signal number.
     */
    struct ExitStatus
    {
        bool exitedNormally{false};
        std::optional<int> code;
    };

    /**
     * @brief Construct a Process instance.
     *
     * Call start() to create the  child process.
     *
     * @param executable Path to the executable file.
     * @param args Command line arguments passed to the executable.
     */
    explicit Process(std::filesystem::path exec, const std::vector<std::string>& args = {});

    /**
     * @brief Destroy the Process object.
     *
     * If the process is still running, it is forcibly terminated using stop().
     */
    ~Process();

    // Process handle is a unique resource
    Process(const Process&) = delete;
    Process(Process&&) = delete;
    Process& operator=(const Process&) = delete;
    Process& operator=(Process&&) = delete;

    /**
     * @brief Start the configured executable.
     *
     * Creates a child process using `fork()` and replaces its image using `execv()`.
     *
     * Startup failure is detected and returned immediately. This includes
     * missing executables, permission errors, and other execv() failures.
     *
     * A Process instance may only be started once, it cannot be restarted.
     *
     * @return true if the process started successfully.
     * @return false if process creation or execution failed.
     */
    bool start();

    /**
     * @brief Sends SIGTERM to the child process
     * "Kindly stop what you are doing"
     */
    void requestStop();

    /**
     * @brief Sends SIGKILL to the child process.
     * "KILL THE BITCH!"
     */
    void stop();

    /**
     * @brief Sends SIGINT to the child process
     * "Ctrl+C to the rescue!"
     */
    void interrupt();

    /**
     * @brief Determine whether the process is still running (using `waitpid()`).
     *
     * If the process has terminated, its exit status is cached and available
     * through wait() and exitStatus().
     *
     * @return true if the process is currently running.
     * @return false otherwise.
     */
    bool isRunning();

    /**
     * @brief Wait for process termination.
     *
     * Blocks until the child process exits.
     *
     * If the process was already collected by isRunning(), the cached exit
     * status is returned immediately.
     *
     * @return Process exit status on success.
     * @return std::nullopt if the process failed before an exit status could
     *         be obtained.
     */

    std::optional<ExitStatus> wait();
    /**
     * @brief Wait for process termination with timeout.
     *
     * Repeatedly checks the process state until either:
     * - The process exits
     * - The timeout expires (busy wait using 1ms increments)
     *
     * @param timeout Maximum duration to wait.
     *
     * @return true if the process exited before the timeout.
     * @return false if the timeout expired.
     *
     * @pre The process has been started.
     */
    bool waitFor(std::chrono::milliseconds timeout);

    /**
     * @brief Retrieve the cached exit status.
     *
     * The exit status becomes available once the process has terminated and
     * been collected (by isRunning() or wait()).
     *
     * @return Cached exit status if available.
     * @return std::nullopt if the process is still running or no status exists.
     *
     * @pre The process has been started.
     */
    [[nodiscard]] std::optional<ExitStatus> exitStatus() const;

    /**
     * @brief Get the operating system process identifier.
     *
     * @return Child process PID.
     *
     * @pre The process has been started.
     */
    [[nodiscard]] pid_t pid() const;

    /**
     * @brief Get the current lifecycle state.
     *
     * @return Current Process::State value.
     */
    [[nodiscard]] State state() const;

  private:
    std::filesystem::path executable_;
    std::vector<std::string> arguments_;
    State state_{State::NotStarted};
    pid_t pid_{-1};
    ExitStatus exitStatus_;

    void startChild(const std::array<int, 2>& pipeFd);
    bool startParent(const std::array<int, 2>& pipeFd);
};

inline Process::Process(std::filesystem::path exec, const std::vector<std::string>& args)
    : executable_(std::move(exec))
    , arguments_(args)
{
}

inline Process::~Process()
{
    if (state_ == State::Running)
        stop();
}

inline bool Process::start()
{
    assert(state_ != State::Exited && "Process can only be started once!");
    if (state_ == State::Running)
        return true;
    if (state_ != State::NotStarted)
        return false;

    // Create pipes to communicate execv failure from child to parent
    std::array<int, 2> pipeFd{};
    if (pipe(pipeFd.data()) != 0)
    {
        state_ = State::Failed;
        return false;
    }
    // Set pipe to close on execv, to avoid parent waiting on child,
    // that no longer has access to the pipe
    fcntl(pipeFd[0], F_SETFD, FD_CLOEXEC);
    fcntl(pipeFd[1], F_SETFD, FD_CLOEXEC);

    // Fork the process into parent and child
    pid_ = fork();

    // Failed fork
    if (pid_ < 0)
    {
        close(pipeFd[0]);
        close(pipeFd[1]);

        state_ = State::Failed;
        return false;
    }

    // Finish start() for the child process
    if (pid_ == 0)
        startChild(pipeFd);

    // Finish start() for the parent process
    return startParent(pipeFd);
}

inline void Process::requestStop()
{
    if (state_ != State::Running)
        return;
    [[maybe_unused]]
    int result = kill(pid_, SIGTERM);
}

inline void Process::stop()
{
    if (state_ != State::Running)
        return;
    [[maybe_unused]]
    int result = kill(pid_, SIGKILL);
    wait();
}

inline void Process::interrupt()
{
    if (state_ != State::Running)
        return;
    [[maybe_unused]]
    int result = kill(pid_, SIGINT);
}

inline bool Process::isRunning()
{
    if (state_ != State::Running)
        return false;

    int status{};
    pid_t result = waitpid(pid_, &status, WNOHANG);

    if (result == 0)
    {
        return true;
    }
    if (result == pid_)
    {
        if (WIFEXITED(status))
        {
            exitStatus_.exitedNormally = true;
            exitStatus_.code = WEXITSTATUS(status);
        }
        else if (WIFSIGNALED(status))
        {
            exitStatus_.exitedNormally = false;
            exitStatus_.code = WTERMSIG(status);
        }

        state_ = State::Exited;

        return false;
    }

    state_ = State::Failed;

    return false;
}

inline std::optional<Process::ExitStatus> Process::wait()
{
    assert(state_ != State::NotStarted);
    if (state_ == State::Exited)
        return exitStatus_;
    if (state_ == State::Failed)
        return {};

    int status{};
    [[maybe_unused]]
    pid_t result = waitpid(pid_, &status, 0);

    state_ = State::Exited;
    if (WIFEXITED(status))
    {
        exitStatus_.exitedNormally = true;
        exitStatus_.code = WEXITSTATUS(status);
    }
    else if (WIFSIGNALED(status))
    {
        exitStatus_.exitedNormally = false;
        exitStatus_.code = WTERMSIG(status);
    }

    return {exitStatus_};
}

inline bool Process::waitFor(std::chrono::milliseconds timeout)
{
    assert(state_ != State::NotStarted);
    auto now = std::chrono::steady_clock::now;
    auto deadline = now() + timeout;

    while (now() < deadline)
    {
        if (!isRunning())
            return true;
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    return false;
}

inline std::optional<Process::ExitStatus> Process::exitStatus() const
{
    assert(state_ != State::NotStarted);
    if (state_ != State::Exited)
        return {};
    return {exitStatus_};
}

inline pid_t Process::pid() const
{
    assert(state_ != State::NotStarted);
    return pid_;
}

inline Process::State Process::state() const
{
    return state_;
}

inline void Process::startChild(const std::array<int, 2>& pipeFd)
{
    // Close read pipe, the child does not need it
    close(pipeFd[0]);

    std::string executableString = executable_.string();

    std::vector<char*> argv;
    argv.reserve(arguments_.size() + 2); // +1 for command +1 for nullptr

    argv.push_back(executableString.data());
    for (auto& arg : arguments_)
    {
        argv.push_back(arg.data());
    }
    argv.push_back(nullptr);

    execv(executable_.c_str(), argv.data());

    // If execv was not succesfull, send error through pipe
    int error = errno;
    write(pipeFd[1], &error, sizeof(error));
    close(pipeFd[1]);

    _exit(127);
}

inline bool Process::startParent(const std::array<int, 2>& pipeFd)
{
    // Close write pipe, the parent does not need it
    close(pipeFd[1]);

    // Wait on reading the pipe or until the pipe closes
    int error{};
    ssize_t bytes = read(pipeFd[0], &error, sizeof(error));
    close(pipeFd[0]);

    // If something is sent via the pipe the execv in the child failed
    if (bytes > 0)
    {
        // Wait for the child to exit
        [[maybe_unused]]
        pid_t result = waitpid(pid_, nullptr, 0);

        pid_ = -1;
        state_ = State::Failed;

        return false;
    }

    // Successfully created the child process
    state_ = State::Running;

    return true;
}

} // namespace process
