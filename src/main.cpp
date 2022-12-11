#include <drawing/SplashWindow.h>
#include "drawing/SplashScene.h"
#include <future>
#include <thread>
#include "state_transitions/WindowStateMachineImpl.h"
#include "resource_managers/TextureLoaderImpl.h"

#include "state_transitions/Game.h"
#include "lua/Bootstrapper.h"
#include "resource_managers/FontLoaderImpl.h"
#include "drawing/animations/AnimationPlayerImpl.h"

constexpr auto luaScript = R"(
function onInit(self)
    print("Hello from lua!")
end

function onInit2(self)
end

local count = 0
local time = 0

local function onUpdate(self, delta)
    count = count + 1
    time = time + delta
    if time > 1 then
        self.text = "FPS: " .. tostring(count)
        count = 0
        time = 0
        local function currentSizeUpdater(value)
            self.fillColor = Color.new(255, 255, 255, math.floor(value))
        end
        local animation = Linear.new(currentSizeUpdater, 1, 0, 255)
        playAnimation(animation)
    end
end

local fps = Text.new{text = "FPS: 0", fillColor = Color.new(255, 0, 0, 255), events = {updateEvent = onUpdate}, isWidthManaged = true}

local main = HBox.new{
    events = {
        initEvent = onInit
    },
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
        Layers.new{
            mainLayer = Padding.new{
                child = VBox.new{
                    contentAlignment = VBoxContentAlignment.Right,
                    horizontalSizeMode = SizeMode.WrapChildren,
                    verticalSizeMode = SizeMode.Managed,
                    spacing = 10,
                    children = {
                        fps,
                        Text.new{text = "Hello world!", characterSize = 20},
                        Text.new{text = "Hello world!", characterSize = 20, fillColor = Color.new(255, 0, 0, 255), isWidthManaged = true}
                    }
                },
                left = 10,
                top = 10,
                right = 10,
                bottom = 10
            },
            children = {
                Quad.new{isWidthManaged = true, isHeightManaged = true, fillColor = Color.new(0, 0, 0, 255)},
            }
        }
    }
}
main.initEvent = onInit2
local bg = Quad.new{isWidthManaged = true, isHeightManaged = true, fillColor = Color.new(255, 255, 255, 255)}
local layers = Layers.new{mainLayer = main, children = {bg, main}}

return layers
)";

auto
main() -> int
{
    auto state = sol::state{};
    state.open_libraries(
      sol::lib::jit, sol::lib::base, sol::lib::io, sol::lib::math);

    auto textureLoader =
      std::make_shared<resource_managers::TextureLoaderImpl>();
    auto fontLoader = std::make_shared<resource_managers::FontLoaderImpl>();
    auto animationPlayer = drawing::animations::AnimationPlayerImpl{};
    auto startingScene = std::make_shared<
      drawing::SplashScene<events::Signals2Event,
                           drawing::animations::AnimationPlayerImpl,
                           resource_managers::TextureLoaderImpl,
                           resource_managers::FontLoaderImpl>>(
      std::move(state),
      std::move(animationPlayer),
      textureLoader,
      fontLoader,
      luaScript);

    auto startingWindow = std::make_shared<drawing::SplashWindow>(
      std::move(startingScene), sf::VideoMode{ 800, 600 }, "RhythmGame");
    auto windowStateMachine = state_transitions::WindowStateMachineImpl{};
    auto game = state_transitions::Game{ std::move(windowStateMachine),
                                         std::move(startingWindow) };
    game.run();

    return 0;
}
