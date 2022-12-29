//
// Created by bobini on 27.12.22.
//

#ifndef RHYTHMGAME_FINDASSETSFOLDERBOOST_H
#define RHYTHMGAME_FINDASSETSFOLDERBOOST_H
#include "AssetsFolderFinder.h"
namespace resource_managers {
[[nodiscard]] auto
findAssetsFolder() -> std::filesystem::path;
static_assert(AssetsFolderFinder<decltype(findAssetsFolder)>);
} // namespace resource_managers
#endif // RHYTHMGAME_FINDASSETSFOLDERBOOST_H
