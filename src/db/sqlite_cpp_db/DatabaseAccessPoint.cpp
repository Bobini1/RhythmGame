//
// Created by bobini on 17.09.22.
//

#include "DatabaseAccessPoint.h"
auto
DatabaseAccessPoint::operator[](const std::string& dbPath) -> SQLite::Database&
{
    if (!connections.contains(dbPath)) {
        connections.emplace(
          dbPath,
          SQLite::Database(
            dbPath,
            SQLite::OPEN_READWRITE | // NOLINT(hicpp-signed-bitwise)
              SQLite::OPEN_CREATE));
        connections.at(dbPath).exec("PRAGMA journal_mode=WAL;");
        connections.at(dbPath).exec("PRAGMA synchronous=NORMAL;");
#ifdef DEBUG
        connections.at(dbPath).exec("PRAGMA foreign_keys=ON;");
#endif
    }
    return connections.at(dbPath);
}
auto
DatabaseAccessPoint::at(const std::string& dbPath) const
  -> const SQLite::Database&
{
    return connections.at(dbPath);
}
