//
// Created by bobini on 28.12.22.
//

#ifndef RHYTHMGAME_QMLSCRIPTFINDERIMPL_H
#define RHYTHMGAME_QMLSCRIPTFINDERIMPL_H
#include <map>
#include "resource_managers/QmlScriptFinder.h"
#include "resource_managers/AssetsFolderFinder.h"

namespace resource_managers {
class QmlScriptFinderImpl
{
    std::filesystem::path rootFolder;
    std::map<std::string, std::string> redirects;

  public:
    explicit QmlScriptFinderImpl(std::filesystem::path rootFolder,
                                 std::map<std::string, std::string> redirects);
    [[nodiscard]] auto operator()(std::string scriptName) const
      -> std::filesystem::path;
};
static_assert(QmlScriptFinder<QmlScriptFinderImpl>);
} // namespace resource_managers

#endif // RHYTHMGAME_QMLSCRIPTFINDERIMPL_H
