#include <drawing/SplashWindow.h>
#include "drawing/SplashScene.h"
#include <future>
#include <thread>
#include "drawing/Window.h"
#include "resource_managers/TextureLoaderImpl.h"

#include "state_transitions/Game.h"
#include "lua/Bootstrapper.h"
#include "resource_managers/FontLoaderImpl.h"
#include "drawing/animations/AnimationPlayerImpl.h"

constexpr auto luaScript = R"(
function dump(o)
   if type(o) == 'table' then
      local s = '{ '
      for k,v in pairs(o) do
         if type(k) ~= 'number' then k = '"'..k..'"' end
         s = s .. '['..k..'] = ' .. dump(v) .. ','
      end
      return s .. '} '
   else
      return tostring(o)
   end
end

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
            self.fillColor = {r=255, g=255, b=255, a=math.floor(value)}
        end
        local animation = Linear.new(currentSizeUpdater, 1, 0, 255)
        playAnimation(animation)
    end
end
local fps = Text.new{text = "FPS: 0", fillColor = {r=255, g=0, b=0, a=255}, onUpdate = onUpdate, isWidthManaged = true}

local main = HBox.new{
    onInit = onInit,
    contentAlignment = HBoxContentAlignment.Bottom,
    horizontalSizeMode = SizeMode.Managed,
    verticalSizeMode = SizeMode.Managed,
    children = {
        Quad.new{width = 100, isHeightManaged = true, fillColor = {r=255, g=0, b=255, a=255}},
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
                        Text.new{text = "Hello world!", characterSize = 20, fillColor = {r=255, g=0, b=0, a=255}, isWidthManaged = true,
                            onMouseEnter = function(self)
                                self.text = "Mouse entered!"
                            end,
                            onMouseLeave = function(self)
                                self.text = "Mouse left!"
                            end,
                            onLeftClick = function(self)
                                self.text = "Mouse clicked!"
                            end
                        }
                    }
                },
                left = 10,
                top = 10,
                right = 10,
                bottom = 10,
                onMouseEnter = function(self)
                    print("Hovered")
                end,
                onLeftClick = function(self)
                    self.parent:getChild(1).fillColor = {r=0, g=255, b=0, a=255}
                end
            },
            children = {
                Quad.new{isWidthManaged = true, isHeightManaged = true, fillColor = {r=0, g=0, b=0, a=255}},
            }
        }
    }
}
main.onInit = onInit2
local bg = Quad.new{isWidthManaged = true, isHeightManaged = true, fillColor = {255, 255, b=255, a=255}}
local layers = Layers.new{mainLayer = main, children = {bg, main}}

return layers
)";

auto
main() -> int
{
    try {
        auto state = sol::state{};
        state.open_libraries(
          sol::lib::jit, sol::lib::base, sol::lib::io, sol::lib::math);

        auto textureLoader =
          std::make_shared<resource_managers::TextureLoaderImpl>();
        auto fontLoader = std::make_shared<resource_managers::FontLoaderImpl>();
        auto animationPlayer = drawing::animations::AnimationPlayerImpl{};
        auto startingScene = std::make_shared<
          drawing::SplashScene<drawing::animations::AnimationPlayerImpl,
                               resource_managers::TextureLoaderImpl,
                               resource_managers::FontLoaderImpl>>(
          std::move(state),
          std::move(animationPlayer),
          textureLoader,
          fontLoader,
          luaScript);

        auto startingWindow = std::make_shared<drawing::SplashWindow>(
          std::move(startingScene), sf::VideoMode{ 800, 600 }, "RhythmGame");
        auto game = state_transitions::Game{ std::move(startingWindow) };
        game.run();
    } catch (const std::exception& e) {
        spdlog::error("Fatal error: {}", e.what());
        return 1;
    } catch (...) {
        spdlog::error("Fatal error: unknown");
        return 1;
    }

    return 0;
}
