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
local root = HBox.new{
    contentAlignment = HBoxContentAlignment.Bottom,
    horizontalSizeMode = SizeMode.Managed,
    verticalSizeMode = SizeMode.Managed,
    children = {
        Quad.new{width = 100, isHeightManaged = true, fillColor = Color.new(255, 0, 255, 255)},
        Align.new(
            Padding.new{
                child = Sprite.new{
                    texture = "/home/bobini/Pulpit/300645106_5308977632548971_2330521311896357614_n.jpg",
                    width = 200,
                    height = 200
                },
                left = 250
            },
            AlignMode.Center
        ),
        VBox.new{
            contentAlignment = VBoxContentAlignment.Right,
            horizontalSizeMode = SizeMode.WrapChildren,
            verticalSizeMode = SizeMode.Managed,
            children = {
                Text.new("Hello world!"),
                Text.new{text = "Hello world!", characterSize = 20},
                Text.new{text = "Hello world!", characterSize = 20, fillColor = Color.new(255, 0, 0, 255), isWidthManaged = true}
            }
        }
    }
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
      root.get<drawing::actors::Actor*>()->shared_from_this());
    auto startingWindow = std::make_shared<drawing::SplashWindow>(
      std::move(startingScene), sf::VideoMode{ 800, 600 }, "RhythmGame");
    auto windowStateMachine = state_transitions::WindowStateMachineImpl{};
    auto game = state_transitions::Game{ std::move(windowStateMachine),
                                         std::move(startingWindow) };
    game.run();

    return 0;
}
