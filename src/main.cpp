#include <drawing/SplashWindow.h>
#include "drawing/SplashScene.h"
#include <future>
#include <thread>
#include "state_transitions/WindowStateMachineImpl.h"
#include "resource_managers/TextureLoaderImpl.h"

#include "state_transitions/Game.h"
#include "lua/Bootstrapper.h"
#include "resource_managers/FontLoaderImpl.h"

constexpr auto luaScript = R"(
    local quad = Quad.new(100, 100)
    quad.minWidth = 100
    quad.minHeight = 100
    quad.isWidthManaged = true
    quad.isHeightManaged = true
    return quad
)";

auto
main() -> int
{
    sol::state state;
    state.open_libraries(sol::lib::jit, sol::lib::base, sol::lib::io);
    lua::Bootstrapper const bootstrapper;
    bootstrapper.defineCommonTypes(state);

    auto textureLoader = resource_managers::TextureLoaderImpl{};
    bootstrapper.bindTextureLoader(state, textureLoader);

    auto fontLoader = resource_managers::FontLoaderImpl{};
    bootstrapper.bindFontLoader(state, fontLoader);

    auto root = state.script(luaScript);
    auto startingScene = std::make_shared<drawing::SplashScene>(
      root.get<drawing::actors::Actor*>()->shared_from_this());
    auto startingWindow = std::make_shared<drawing::SplashWindow>(
      std::move(startingScene), sf::VideoMode{ 800, 600 }, "RhythmGame");
    auto windowStateMachine = state_transitions::WindowStateMachineImpl{};
    auto game = state_transitions::Game{ std::move(windowStateMachine),
                                         std::move(startingWindow) };
    game.run();

    return 0;
}
