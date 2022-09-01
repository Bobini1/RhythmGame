//
// Created by bobini on 31.07.2022.
//

#ifndef RHYTHMGAME_LUASCRIPTFINDER_H
#define RHYTHMGAME_LUASCRIPTFINDER_H

#include <string>
#include <filesystem>
namespace resource_locators {
template<typename T>
concept LuaScriptFinder = requires(T luaScriptFinder, const std::string& screen)
{
    {luaScriptFinder.findHandlerScript(screen)} -> std::convertible_to<std::string>;
};
} // namespace resource_locators

#endif // RHYTHMGAME_LUASCRIPTFINDER_H
