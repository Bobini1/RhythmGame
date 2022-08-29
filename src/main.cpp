#include "wiring/Injector.h"

#include "state_transitions/Game.h"

auto
main() -> int
{
    auto injector = wiring::getInjector();

    auto game = injector.create<state_transitions::Game>();
    game.run();

    return 0;
}
