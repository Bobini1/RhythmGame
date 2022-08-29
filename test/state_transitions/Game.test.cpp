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

class DummyWindowManager : public state_transitions::WindowStateMachine
{
    std::shared_ptr<drawing::Window> current;

  public:
    auto changeWindow(std::shared_ptr<drawing::Window> window) -> void override
    {
        current = std::move(window);
    }
    auto update(std::chrono::nanoseconds delta) -> void override
    {
        current->update(delta);
    }
    auto draw() const -> void override { current->draw(); }
    auto pollEvents() -> void override {}
    auto isOpen() -> bool override { return current->isOpen(); }
    auto getCurrentWindow() -> std::shared_ptr<drawing::Window> override
    {
        return current;
    }
};
} // namespace

TEST_CASE("The game can run without issues for a few frames",
          "[state_transitions][.window]")
{
    auto game = state_transitions::Game{
        std::make_shared<DummyWindowManager>(),
        std::make_shared<DummyWindow>(),
    };
    REQUIRE_NOTHROW(game.run());
}