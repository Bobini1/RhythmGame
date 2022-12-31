//
// Created by bobini on 31.07.2022.
//

#ifndef RHYTHMGAME_LUASCRIPTFINDER_H
#define RHYTHMGAME_LUASCRIPTFINDER_H

#include <string>
#include <filesystem>
namespace resource_managers {
/**
 * @brief Finds Lua script paths for specific screens.
 */
template<typename T>
concept LuaScriptFinder =
  std::is_same_v<std::invoke_result_t<T, std::string>, std::filesystem::path>;
} // namespace resource_managers

#endif // RHYTHMGAME_LUASCRIPTFINDER_H
