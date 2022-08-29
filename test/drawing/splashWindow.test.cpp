//
// Created by bobini on 21.08.22.
//

#include "drawing/Scene.h"
#include "drawing/SplashScene.h"
#include "drawing/SplashWindow.h"
#include <SFML/Window/Event.hpp>
#include <catch2/catch_test_macros.hpp>

namespace {
class DummyScene : public drawing::Scene
{
    unsigned updateCount{};
    mutable unsigned drawCount{};

  protected:
    void draw(sf::RenderTarget& target, sf::RenderStates states) const override
    {
        drawCount++;
    }

  public:
    auto update(std::chrono::nanoseconds /*delta*/) -> void override
    {
        updateCount++;
    }
    auto getUpdateCount() const -> unsigned { return updateCount; }
    auto getDrawCount() const -> unsigned { return drawCount; }
};
} // namespace

TEST_CASE("the splash window draws its scene", "[drawing][window][!mayfail]")
{
    auto dummyScene = std::make_shared<DummyScene>();
    auto sceneStateMachine = std::make_shared<drawing::SplashWindow>(
      dummyScene, sf::VideoMode{ 800, 600 }, "Splash Window");
    sceneStateMachine->draw();
    REQUIRE(dummyScene->getDrawCount() == 1);
    sceneStateMachine->update(std::chrono::nanoseconds(1));
    REQUIRE(dummyScene->getUpdateCount() == 1);
}