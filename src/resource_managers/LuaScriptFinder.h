//
// Created by bobini on 31.07.2022.
//

#ifndef RHYTHMGAME_LUASCRIPTFINDER_H
#define RHYTHMGAME_LUASCRIPTFINDER_H

#include <string>
#include <filesystem>
namespace resource_managers {
/**
 * @brief Finds Lua screen paths for specific screens.
 */
template<typename T>
concept LuaScriptFinder =
  requires(T luaScriptFinder, const std::string& screen) {
      /**
       * @brief Finds Lua script path for a screen.
       * @param screen Name of the screen.
       * @return Path to the Lua script.
       */
      {
          luaScriptFinder.findHandlerScript(screen)
      } -> std::convertible_to<std::string>;
  };
} // namespace resource_managers

#endif // RHYTHMGAME_LUASCRIPTFINDER_H
