#include <utility>
#include <co/co.h>
#include <limits>
#include <iostream>

void
f()
{
    int x{};
    co::Chan<int> chan(1, std::numeric_limits<uint32_t>::max());
    go([chan] { chan << 1; });
    chan >> x;
    std::cout << x << std::endl;
}

auto
mainCo() -> int
{
    f();
    return 0;
}

auto
main() -> int
{
    co::WaitGroup wg;
    wg.add();
    int ret;
    go([&]() { ret = mainCo(); });
    wg.wait();

    return ret;
}
