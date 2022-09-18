//
// Created by bobini on 17.09.22.
//

#ifndef RHYTHMGAME_DATABASEACCESSPOINT_H
#define RHYTHMGAME_DATABASEACCESSPOINT_H

#include <string>
#include <map>
#include <SQLiteCpp/Database.h>
class DatabaseAccessPoint
{
    std::map<std::string, SQLite::Database> connections;

  public:
    auto operator[](const std::string& dbPath) -> SQLite::Database&;
    auto at(const std::string& dbPath) const -> const SQLite::Database&;
};

#endif // RHYTHMGAME_DATABASEACCESSPOINT_H
