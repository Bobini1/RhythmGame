#include <co/co.h>

#include "wiring/Injector.h"

#include "state_transitions/Game.h"

auto
mainCo() -> int
{
    auto injector = wiring::getInjector();

    auto game = injector.create<state_transitions::Game>();
    game.run();

    return 0;
}

auto
main() -> int
{
    co::WaitGroup waitGroup;
    waitGroup.add();
    int ret{};
    go([&]() { ret = mainCo(); });
    waitGroup.wait();

    return ret;
}
