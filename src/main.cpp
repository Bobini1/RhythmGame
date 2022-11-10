#include <drawing/SplashWindow.h>
#include "drawing/SplashScene.h"
#include <future>
#include <thread>
#include "state_transitions/WindowStateMachineImpl.h"

#include "state_transitions/Game.h"
#include "lua/Bootstrapper.h"

constexpr auto luaScript = R"(
    local root = VBox.new()
    local quad = Quad.new()
    quad.width = 100
    quad.height = 100
    quad.fillColor = Color.new(255, 0, 0, 255)
    local quad2 = Quad.new()
    quad2.width = 100
    quad2.height = 100
    quad2.fillColor = Color.new(0, 255, 0, 255)
    local quad3 = Quad.new()
    quad3.width = 100
    quad3.height = 100
    quad3.fillColor = Color.new(0, 0, 255, 255)
    root:addChild(quad)
    root:addChild(quad2)
    root:addChild(quad3)
    root:getChild(1).width = 200
    return root
)";
auto
main() -> int
{
    sol::state state;
    state.open_libraries(sol::lib::jit, sol::lib::base, sol::lib::io);
    lua::Bootstrapper bootstrapper;
    bootstrapper.defineTypes(state);
    auto root = state.script(luaScript);
    auto startingScene =
      std::make_shared<drawing::SplashScene>(root.get<drawing::actors::VBox*>()->shared_from_this());
    auto startingWindow = std::make_shared<drawing::SplashWindow>(
      std::move(startingScene), sf::VideoMode{ 800, 600 }, "RhythmGame");
    auto windowStateMachine = state_transitions::WindowStateMachineImpl{};
    auto game = state_transitions::Game{ std::move(windowStateMachine),
                                         std::move(startingWindow) };
    game.run();

    return 0;
}
