//
// Created by bobini on 14.09.23.
//

#ifndef RHYTHMGAME_SONGDBSCANNER_H
#define RHYTHMGAME_SONGDBSCANNER_H

#include <span>
#include "db/SqliteCppDb.h"
#include "ChartDataFactory.h"
namespace resource_managers {

class SongDbScanner
{
    db::SqliteCppDb* db;

  public:
    explicit SongDbScanner(db::SqliteCppDb* db);
    void scanDirectories(std::span<const std::filesystem::path> directories);
};

} // namespace resource_managers

#endif // RHYTHMGAME_SONGDBSCANNER_H
