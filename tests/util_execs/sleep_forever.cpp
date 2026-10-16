#include <unistd.h>

int main()
{
    while (42)
    {
        ::sleep(1);
    }

    return 123;
}
