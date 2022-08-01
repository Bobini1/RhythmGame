//
// Created by bobini on 31.07.2022.
//

#ifndef RHYTHMGAME_LUASCRIPTFINDER_H
#define RHYTHMGAME_LUASCRIPTFINDER_H

#include <string>
#include <filesystem>
namespace resource_locators {
class LuaScriptFinder
{
  public:
    virtual auto findHandlerScript(const std::string& screen)
      -> std::string = 0;

    virtual ~LuaScriptFinder() = default;
};
} // namespace resource_locators

#endif // RHYTHMGAME_LUASCRIPTFINDER_H
