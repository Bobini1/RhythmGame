//
// Created by bobini on 16.04.23.
//

#ifndef RHYTHMGAME_FINDTESTASSETSFOLDER_H
#define RHYTHMGAME_FINDTESTASSETSFOLDER_H

#include <array>
#include <filesystem>
inline auto
findTestAssetsFolder() -> std::filesystem::path
{
    static const auto assetsFolder = []() -> std::filesystem::path {
        const auto current = std::filesystem::current_path();
        const std::array candidates {
            current / "testOnlyAssets",
            current.parent_path() / "testOnlyAssets",
            std::filesystem::path(__FILE__).parent_path().parent_path() /
              "testOnlyAssets",
        };
        for (const auto& candidate : candidates) {
            if (std::filesystem::exists(candidate)) {
                return candidate;
            }
        }
        return candidates.front();
    }();
    return assetsFolder;
}

#endif // RHYTHMGAME_FINDTESTASSETSFOLDER_H
