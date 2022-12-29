//
// Created by bobini on 28.12.22.
//

#ifndef RHYTHMGAME_LUASCRIPTFINDERIMPL_H
#define RHYTHMGAME_LUASCRIPTFINDERIMPL_H
#include <map>
#include "resource_managers/LuaScriptFinder.h"
#include "resource_managers/AssetsFolderFinder.h"

namespace resource_managers {
class LuaScriptFinderImpl
{
    std::filesystem::path rootFolder;
    std::map<std::string, std::string> redirects;

  public:
    explicit LuaScriptFinderImpl(std::filesystem::path rootFolder,
                                 std::map<std::string, std::string> redirects);
    [[nodiscard]] auto operator()(std::string scriptName) const
      -> std::filesystem::path;
};
static_assert(LuaScriptFinder<LuaScriptFinderImpl>);
} // namespace resource_managers

#endif // RHYTHMGAME_LUASCRIPTFINDERIMPL_H
