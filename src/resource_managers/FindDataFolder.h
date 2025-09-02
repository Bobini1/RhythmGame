//
// Created by bobini on 27.12.22.
//

#ifndef RHYTHMGAME_FINDDATAFOLDER_H
#define RHYTHMGAME_FINDDATAFOLDER_H
#include <filesystem>
namespace resource_managers {
auto
findDataFolder() -> std::filesystem::path;
} // namespace resource_managers
#endif // RHYTHMGAME_FINDDATAFOLDER_H
