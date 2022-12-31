//
// Created by bobini on 28.12.22.
//

#include "LuaScriptFinderImpl.h"
resource_managers::LuaScriptFinderImpl::LuaScriptFinderImpl(
  std::filesystem::path rootFolder,
  std::map<std::string, std::string> redirects)
  : rootFolder(std::move(rootFolder))
  , redirects(std::move(redirects))
{
}
auto
resource_managers::LuaScriptFinderImpl::operator()(std::string scriptName) const
  -> std::filesystem::path
{
    if (redirects.contains(scriptName)) {
        scriptName = redirects.at(scriptName);
    }
    return rootFolder / scriptName;
}
