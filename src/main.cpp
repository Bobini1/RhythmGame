#include <drawing/SplashWindow.h>
#include "drawing/SplashScene.h"
#include "drawing/Window.h"
#include "resource_managers/TextureLoaderImpl.h"

#include "state_transitions/Game.h"
#include "lua/Bootstrapper.h"
#include "resource_managers/FontLoaderImpl.h"
#include "drawing/animations/AnimationPlayerImpl.h"
#include "resource_managers/FindAssetsFolderBoost.h"
#include "resource_managers/LoadConfig.h"
#include "resource_managers/LuaScriptFinderImpl.h"

#include <gstreamer-1.0/gst/gst.h>

auto
loadGame(resource_managers::LuaScriptFinder auto luaScriptFinder,
         resource_managers::FontLoader auto fontLoader,
         resource_managers::TextureLoader auto textureLoader) -> void
{
    auto state = sol::state{};
    state.open_libraries(
      sol::lib::jit, sol::lib::base, sol::lib::io, sol::lib::math);

    auto scriptPath = luaScriptFinder("Main");

    auto animationPlayer = drawing::animations::AnimationPlayerImpl{};
    auto startingScene =
      std::make_shared<drawing::SplashScene<decltype(animationPlayer),
                                            decltype(textureLoader),
                                            decltype(fontLoader)>>(
        std::move(state),
        std::move(animationPlayer),
        &textureLoader,
        &fontLoader,
        std::move(scriptPath));

    auto startingWindow = std::make_shared<drawing::SplashWindow>(
      std::move(startingScene), sf::VideoMode{ 800, 600 }, "RhythmGame");
    auto game = state_transitions::Game{ std::move(startingWindow) };
    game.run();
}

auto
main() -> int
{
    gst_init(nullptr, nullptr);
    // install plugins
    gst_registry_scan_path(gst_registry_get(), "/usr/lib/gstreamer-1.0");

    // get loaded plugins
    auto plugins = gst_registry_get_plugin_list(gst_registry_get());
    for (auto plugin = plugins; plugin != nullptr;
         plugin = g_list_next(plugin)) {
        auto pluginName = gst_plugin_get_name(GST_PLUGIN(plugin->data));
        std::cout << "Plugin: " << pluginName << std::endl;
    }
    g_list_free(plugins);
    std::cout << "Done" << std::endl;
    // print GST_PLUGIN_PATH
    auto pluginPath = g_getenv("GST_PLUGIN_PATH");
    if (pluginPath != nullptr) {
        std::cout << "GST_PLUGIN_PATH: " << pluginPath << std::endl;
    } else {
        std::cout << "GST_PLUGIN_PATH not set" << std::endl;
    }
    try {
        auto assetsFolder = resource_managers::findAssetsFolder();
        auto textureConfig = resource_managers::loadConfig(
          assetsFolder / "themes" / "Default" / "textures" / "textures.ini");
        auto fontConfig = resource_managers::loadConfig(
          assetsFolder / "themes" / "Default" / "fonts" / "fonts.ini");
        auto scriptConfig = resource_managers::loadConfig(
          assetsFolder / "themes" / "Default" / "scripts" / "scripts.ini");
        auto fontManager =
          resource_managers::FontLoaderImpl{ assetsFolder / "themes" /
                                               "Default" / "fonts",
                                             fontConfig["FontNames"] };
        auto textureManager =
          resource_managers::TextureLoaderImpl{ assetsFolder / "themes" /
                                                  "Default" / "textures",
                                                textureConfig["TextureNames"] };
        auto luaScriptFinder =
          resource_managers::LuaScriptFinderImpl{ assetsFolder / "themes" /
                                                    "Default" / "scripts",
                                                  scriptConfig["ScriptNames"] };
        loadGame(std::move(luaScriptFinder),
                 std::move(fontManager),
                 std::move(textureManager));

    } catch (const std::exception& e) {
        spdlog::error("Fatal error: {}", e.what());
        return 1;
    } catch (...) {
        spdlog::error("Fatal error: unknown");
        return 1;
    }

    return 0;
}
