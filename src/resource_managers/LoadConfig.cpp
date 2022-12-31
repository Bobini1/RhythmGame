//
// Created by bobini on 28.12.22.
//

#include "LoadConfig.h"
auto
resource_managers::loadConfig(const std::filesystem::path& path)
  -> std::map<std::string, std::map<std::string, std::string>>
{
    boost::property_tree::ptree ptree;
    boost::property_tree::read_ini(path.string(), ptree);
    std::map<std::string, std::map<std::string, std::string>> result;
    for (const auto& section : ptree) {
        std::map<std::string, std::string> sectionMap;
        for (const auto& key : section.second) {
            sectionMap[key.first] = key.second.data();
        }
        result[section.first] = std::move(sectionMap);
    }
    return result;
}
