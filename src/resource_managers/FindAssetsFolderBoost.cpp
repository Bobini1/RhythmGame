//
// Created by bobini on 27.12.22.
//

#include "FindAssetsFolderBoost.h"
#include "boost/dll/runtime_symbol_info.hpp"
auto
resource_managers::findAssetsFolder() -> std::filesystem::path
{
    static const auto assetsFolder =
#ifdef _WIN32
      std::filesystem::path(boost::dll::program_location().wstring())
#else
      std::filesystem::path(boost::dll::program_location().string())
#endif
        .parent_path()
        .parent_path() /
      "assets";
    return assetsFolder;
}
