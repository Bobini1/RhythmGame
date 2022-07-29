#include <utility>
#include <co/co.h>
#include <limits>
#include <iostream>

auto
mainCo() -> int
{
    auto chan = co::Chan<int>();
    go([chan]() {
        int message{};
        chan >> message;
        std::cout << message << std::endl;
    });
    go([chan]() {
        chan << 123;
        co::sleep(1000);
        chan << 456;
    });
    return 0;
}

auto
main() -> int
{
    auto socket = co::socket(AF_INET, SOCK_STREAM, 0);
    co::WaitGroup wg;
    wg.add();
    int ret;
    go([&]() { ret = mainCo(); });
    wg.wait();

    return ret;
}
