//
// Created by bobini on 27.12.22.
//

#include "FindAssetsFolderBoost.h"
#include "boost/dll/runtime_symbol_info.hpp"
auto
resource_managers::findAssetsFolder() -> std::filesystem::path
{
    static const auto assetsFolder = boost::dll::program_location()
                                       .parent_path()
                                       .parent_path()
                                       .append("assets")
                                       .string();
    return assetsFolder;
}
