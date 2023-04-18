//
// Created by bobini on 16.04.23.
//

#ifndef RHYTHMGAME_FINDTESTASSETSFOLDER_H
#define RHYTHMGAME_FINDTESTASSETSFOLDER_H
#include <filesystem>
#include <boost/dll/runtime_symbol_info.hpp>
inline auto
findTestAssetsFolder() -> std::filesystem::path
{
    static const auto assetsFolder =
      std::filesystem::path(boost::dll::program_location().c_str())
        .parent_path()
        .parent_path() /
      "testOnlyAssets";
    return assetsFolder;
}

#endif // RHYTHMGAME_FINDTESTASSETSFOLDER_H
