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
    DummyWindow()
      : drawing::Window(sf::VideoMode(1, 1), "", sf::Style::None)
    {
    }
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

TEST_CASE("The game can run without issues for a few frames",
          "[state_transitions][.window]")
{
    auto game = state_transitions::Game{
        std::make_shared<DummyWindow>(),
    };
    REQUIRE_NOTHROW(game.run());
}