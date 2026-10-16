#include <atomic>
#include <csignal>

std::atomic<bool> stop{false};

void onSigint(int)
{
    stop = true;
}

int main()
{
    signal(SIGINT, onSigint);

    while (!stop)
    {
        sleep(1);
    }

    return 123;
}
