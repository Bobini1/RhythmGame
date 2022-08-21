//
// Created by bobini on 21.08.22.
//

#include "wiring/Injector.h"
#include "state_transitions/Game.h"
#include <catch2/catch_test_macros.hpp>

TEST_CASE("Injector is able to construct the whole program", "[wiring]")
{
    const auto injector = wiring::getInjector();
    STATIC_REQUIRE(
      boost::di::is_creatable<std::unique_ptr<state_transitions::Game>>(
        injector));
}