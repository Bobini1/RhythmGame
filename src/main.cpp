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
    local root = VBox.new{
        Text.new{text = "Hello world!", characterSize = 50, fillColor = Color.new(255, 255, 0, 255)},
        Padding.new(
            Quad.new{width = 100, height = 100, fillColor = Color.new(255, 0, 0, 255)},
            {left = 10, right = 10, top = 10, bottom = 10}
        ),
        Align.new(
            Quad.new{width = 100, height = 100, fillColor = Color.new(0, 255, 0, 255)},
            AlignMode.Center
        ),
        Quad.new{width = 100, height = 100, fillColor = Color.new(0, 0, 255, 255)}
    }
    return root
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
      root.get<drawing::actors::VBox*>()->shared_from_this());
    auto startingWindow = std::make_shared<drawing::SplashWindow>(
      std::move(startingScene), sf::VideoMode{ 800, 600 }, "RhythmGame");
    auto windowStateMachine = state_transitions::WindowStateMachineImpl{};
    auto game = state_transitions::Game{ std::move(windowStateMachine),
                                         std::move(startingWindow) };
    game.run();

    return 0;
}
