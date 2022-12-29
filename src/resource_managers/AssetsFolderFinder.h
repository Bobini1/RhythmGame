//
// Created by bobini on 27.12.22.
//

#ifndef RHYTHMGAME_ASSETSFOLDERFINDER_H
#define RHYTHMGAME_ASSETSFOLDERFINDER_H

#include <type_traits>
#include <filesystem>
namespace resource_managers {

template<typename T>
concept AssetsFolderFinder =
  std::is_same_v<std::invoke_result_t<T>, std::filesystem::path>;

} // namespace resource_managers

#endif // RHYTHMGAME_ASSETSFOLDERFINDER_H
