#include <chrono>

#include <catch2/catch_test_macros.hpp>
#include <csignal>

#include "../process.hpp"

namespace process::tests
{

constexpr const char* kNonexistancExec = "/does/not/exist";
constexpr const char* kExitSuccessExec = "tests/util_execs/exit_success";
constexpr const char* kExit42Exec = "tests/util_execs/exit_42";
constexpr const char* kSleepForeverExec = "tests/util_execs/sleep_forever";
constexpr const char* kExitSigintHandlerExec = "tests/util_execs/sigint_handler";
constexpr const char* kExitCrashExec = "tests/util_execs/crash";

TEST_CASE("Process: start fails for missing executable")
{
    Process p{kNonexistancExec};

    REQUIRE_FALSE(p.start());
    CHECK(p.state() == Process::State::Failed);
}

TEST_CASE("Process: start launched process")
{
    Process p{kSleepForeverExec};

    REQUIRE(p.start());
    CHECK(p.isRunning());
}

TEST_CASE("Process: wait captures exit code")
{
    Process p{kExit42Exec};

    REQUIRE(p.start());
    auto exitStatus = p.wait();
    REQUIRE(exitStatus);
    CHECK(exitStatus->exitedNormally);
    CHECK(exitStatus->code == 42);
}

TEST_CASE("Process: wait returns cached exit code")
{
    Process p{kExit42Exec};

    REQUIRE(p.start());
    while (p.isRunning())
    {
    }
    auto exitStatus = p.wait();
    REQUIRE(exitStatus);
    CHECK(exitStatus->exitedNormally);
    CHECK(exitStatus->code == 42);
}

TEST_CASE("Process: stop sends SIGKILL")
{
    Process p{kSleepForeverExec};

    REQUIRE(p.start());
    p.stop();
    REQUIRE_FALSE(p.isRunning());
    auto exitStatus = p.exitStatus();
    REQUIRE(exitStatus);
    CHECK_FALSE(exitStatus->exitedNormally);
    CHECK(exitStatus->code == SIGKILL);
}

TEST_CASE("Process: requestStop sends SIGTERM")
{
    Process p{kSleepForeverExec};

    REQUIRE(p.start());
    p.requestStop();
    auto exitStatus = p.wait();
    REQUIRE(exitStatus);
    CHECK_FALSE(exitStatus->exitedNormally);
    CHECK(exitStatus->code == SIGTERM);
}

TEST_CASE("Process: interrupt sends SIGINT")
{
    Process p{kExitSigintHandlerExec};

    REQUIRE(p.start());
    p.interrupt();
    auto exitStatus = p.wait();
    REQUIRE(exitStatus);
    CHECK_FALSE(exitStatus->exitedNormally);
    CHECK(exitStatus->code == SIGINT);
}

TEST_CASE("Process: waitFor timeouts")
{
    Process p{kExit42Exec};

    REQUIRE(p.start());
    REQUIRE(p.waitFor(std::chrono::milliseconds(100)));
}

TEST_CASE("Process: waitFor succeeds")
{
    Process p{kExit42Exec};

    REQUIRE(p.start());
    REQUIRE(p.waitFor(std::chrono::milliseconds(1000)));
}

TEST_CASE("Process: PID is valid after start")
{
    Process p{kSleepForeverExec};

    REQUIRE(p.start());
    REQUIRE(p.pid() > 0);
}

} // namespace process::tests
