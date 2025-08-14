//
// Created by bobini on 27.12.22.
//

#ifndef RHYTHMGAME_FINDASSETSFOLDER_H
#define RHYTHMGAME_FINDASSETSFOLDER_H
#include <filesystem>
namespace resource_managers {
[[nodiscard]] auto
findAssetsFolder() -> std::filesystem::path;
} // namespace resource_managers
#endif // RHYTHMGAME_FINDASSETSFOLDER_H
