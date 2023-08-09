//
// Created by bobini on 31.07.2022.
//

#ifndef RHYTHMGAME_QMLSCRIPTFINDER_H
#define RHYTHMGAME_QMLSCRIPTFINDER_H

#include <string>
#include <filesystem>
namespace resource_managers {
/**
 * @brief Finds Lua script paths for specific screens.
 */
template<typename T>
concept QmlScriptFinder =
  std::is_same_v<std::invoke_result_t<T, std::string>, std::filesystem::path>;
} // namespace resource_managers

#endif // RHYTHMGAME_QMLSCRIPTFINDER_H
