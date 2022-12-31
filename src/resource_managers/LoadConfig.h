//
// Created by bobini on 28.12.22.
//

#ifndef RHYTHMGAME_LOADCONFIG_H
#define RHYTHMGAME_LOADCONFIG_H

#include <boost/property_tree/ini_parser.hpp>
#include <map>
#include <filesystem>
namespace resource_managers {
auto
loadConfig(const std::filesystem::path& path)
  -> std::map<std::string, std::map<std::string, std::string>>;
} // namespace resource_managers
#endif // RHYTHMGAME_LOADCONFIG_H
