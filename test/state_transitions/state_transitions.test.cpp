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
    mutable unsigned drawCount{};

  public:
    auto update(std::chrono::nanoseconds /*delta*/) -> void override
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

TEST_CASE("Only the current scene gets updated and drawn",
          "[state_transitions]")
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
    unsigned drawCount{};

  public:
    DummyWindow()
      : drawing::Window(sf::VideoMode(1, 1), "", sf::Style::None)
    {
    }
    auto getUpdateCount() const -> unsigned { return updateCount; }
    auto update(std::chrono::nanoseconds /*delta*/) -> void override
    {
        updateCount++;
    }
    auto draw() -> void override { drawCount++; }
    auto getDrawCount() const -> unsigned { return drawCount; }
};

TEST_CASE("Windows can be added in the state machine and changed")
{
    auto dummyWindow = std::make_shared<DummyWindow>();
    auto windowStateMachine =
      std::make_shared<state_transitions::WindowStateMachineImpl>();
    windowStateMachine->changeWindow(dummyWindow);
    REQUIRE(windowStateMachine->getCurrentWindow() == dummyWindow);
    auto otherWindow = std::make_shared<DummyWindow>();
    windowStateMachine->changeWindow(otherWindow);
    REQUIRE(windowStateMachine->getCurrentWindow() == otherWindow);
}

TEST_CASE("The window manager's updates get passed down", "[state_transitions]")
{
    auto dummyWindow = std::make_shared<DummyWindow>();
    auto windowStateMachine =
      std::make_shared<state_transitions::WindowStateMachineImpl>();
    windowStateMachine->changeWindow(dummyWindow);
    REQUIRE(windowStateMachine->isOpen());
    REQUIRE(windowStateMachine->getCurrentWindow() == dummyWindow);
    windowStateMachine->update(std::chrono::nanoseconds(1));
    windowStateMachine->draw();
    auto otherWindow = std::make_shared<DummyWindow>();
    windowStateMachine->changeWindow(otherWindow);
    REQUIRE(windowStateMachine->getCurrentWindow() == otherWindow);
    windowStateMachine->update(std::chrono::nanoseconds(1));
    windowStateMachine->draw();
    windowStateMachine->update(std::chrono::nanoseconds(1));
    windowStateMachine->draw();
    REQUIRE(dummyWindow->getUpdateCount() == 1);
    REQUIRE(otherWindow->getUpdateCount() == 2);
    REQUIRE(dummyWindow->getDrawCount() == 1);
    REQUIRE(otherWindow->getDrawCount() == 2);
    REQUIRE(windowStateMachine->isOpen());
}