//
// Created by bobini on 20.08.22.
//

#include "state_transitions/SceneStateMachineImpl.h"
#include "state_transitions/WindowStateMachineImpl.h"
#include "drawing/SplashWindow.h"
#include "drawing/SplashScene.h"
#include "wiring/Injector.h"

#include <catch2/catch_test_macros.hpp>

class DummyScene : public drawing::Scene
{
    unsigned updateCount{};

  public:
    auto update(std::chrono::nanoseconds /*delta*/) -> void override
    {
        updateCount++;
    }
    auto getUpdateCount() const -> unsigned { return updateCount; }

  protected:
    void draw(sf::RenderTarget& target, sf::RenderStates states) const override
    {
    }
};

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

TEST_CASE("Only the current scene gets updated", "[state_transitions]")
{
    auto dummyScene = std::make_shared<DummyScene>();
    auto sceneStateMachine =
      std::make_shared<state_transitions::SceneStateMachineImpl>(dummyScene);
    REQUIRE(sceneStateMachine->getCurrentScene() == dummyScene);
    sceneStateMachine->changeScene(dummyScene);
    REQUIRE(sceneStateMachine->getCurrentScene() == dummyScene);
    sceneStateMachine->update(std::chrono::nanoseconds(1));
    auto otherScene = std::make_shared<DummyScene>();
    sceneStateMachine->changeScene(otherScene);
    REQUIRE(sceneStateMachine->getCurrentScene() == otherScene);
    sceneStateMachine->update(std::chrono::nanoseconds(1));
    sceneStateMachine->update(std::chrono::nanoseconds(1));
    REQUIRE(dummyScene->getUpdateCount() == 1);
    REQUIRE(otherScene->getUpdateCount() == 2);
}

class DummyWindow : public drawing::Window
{
    unsigned updateCount{};

  public:
    DummyWindow()
      : drawing::Window(sf::VideoMode(1, 1), "", sf::Style::None)
    {
    }
    auto getUpdateCount() const -> unsigned { return updateCount; }
    auto update(std::chrono::nanoseconds delta) -> void override {}
    auto draw() -> void override {}
};

TEST_CASE("Winndows can be added in the state machine and changed")
{
    auto dummyWindow = std::make_shared<DummyWindow>();
    auto windowStateMachine =
      std::make_shared<state_transitions::WindowStateMachineImpl>(dummyWindow);
    REQUIRE(windowStateMachine->getCurrentWindow() == dummyWindow);
    auto otherWindow = std::make_shared<DummyWindow>();
    windowStateMachine->changeWindow(otherWindow);
    REQUIRE(windowStateMachine->getCurrentWindow() == otherWindow);
}