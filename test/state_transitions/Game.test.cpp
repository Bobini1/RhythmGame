//
// Created by bobini on 22.08.22.
//

#include "state_transitions/Game.h"
#include <catch2/catch_test_macros.hpp>

namespace {
class DummyWindow : public drawing::Window
{
    unsigned count{};

  public:
    void update(std::chrono::nanoseconds /*delta*/) override
    {
        count++;
        if (count == 10U) {
            close();
        }
    }
    auto draw() -> void override {}
};

} // namespace

#ifndef DISABLE_WINDOW_TESTS
TEST_CASE("The game can run without issues for a few frames",
          "[state_transitions]")
{
    auto game = state_transitions::Game{
        std::make_shared<DummyWindow>(),
    };
    REQUIRE_NOTHROW(game.run());
}
#endif