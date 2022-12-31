//
// Created by bobini on 20.08.22.
//

#include "state_transitions/SceneStateMachineImpl.h"
#include "drawing/Window.h"

#include <catch2/catch_test_macros.hpp>

namespace {
class DummyScene : public drawing::Scene
{
    unsigned updateCount{};
    mutable unsigned drawCount{};

  public:
    auto update(std::chrono::nanoseconds /*delta*/, drawing::Window& /*window*/)
      -> void override
    {
        updateCount++;
    }
    auto getUpdateCount() const -> unsigned { return updateCount; }

  protected:
    void draw(sf::RenderTarget& /*target*/,
              sf::RenderStates /*states*/) const override
    {
        drawCount++;
    }
};
} // namespace

TEST_CASE("Scenes can be permanently switched in the scene state machine",
          "[state_transitions]")
{
    auto dummyScene = std::make_shared<DummyScene>();
    auto sceneStateMachine =
      std::make_shared<state_transitions::SceneStateMachineImpl>(dummyScene);
    REQUIRE(sceneStateMachine->getCurrentScene() == dummyScene);
    sceneStateMachine->changeScene(dummyScene);
    REQUIRE(sceneStateMachine->getCurrentScene() == dummyScene);
    auto otherScene = std::make_shared<DummyScene>();
    sceneStateMachine->changeScene(otherScene);
    REQUIRE(sceneStateMachine->getCurrentScene() == otherScene);
}

namespace {
class DummyWindow : public drawing::Window
{
    unsigned updateCount{};
    unsigned drawCount{};

  public:
    auto getUpdateCount() const -> unsigned { return updateCount; }
    auto update(std::chrono::nanoseconds /*delta*/) -> void override
    {
        updateCount++;
    }
    auto draw() -> void override { drawCount++; }
    auto getDrawCount() const -> unsigned { return drawCount; }
};
} // namespace

#ifndef DISABLE_WINDOW_TESTS
TEST_CASE("Only the current scene gets updated and drawn",
          "[state_transitions]")
{
    auto dummyScene = std::make_shared<DummyScene>();
    auto sceneStateMachine =
      state_transitions::SceneStateMachineImpl{ dummyScene };
    auto dummyWindow = DummyWindow{};
    REQUIRE(sceneStateMachine.getCurrentScene() == dummyScene);
    sceneStateMachine.changeScene(dummyScene);
    REQUIRE(sceneStateMachine.getCurrentScene() == dummyScene);
    sceneStateMachine.update(std::chrono::nanoseconds(1), dummyWindow);
    auto otherScene = std::make_shared<DummyScene>();
    sceneStateMachine.changeScene(otherScene);
    REQUIRE(sceneStateMachine.getCurrentScene() == otherScene);
    sceneStateMachine.update(std::chrono::nanoseconds(1), dummyWindow);
    sceneStateMachine.update(std::chrono::nanoseconds(1), dummyWindow);
    REQUIRE(dummyScene->getUpdateCount() == 1);
    REQUIRE(otherScene->getUpdateCount() == 2);
}
#endif