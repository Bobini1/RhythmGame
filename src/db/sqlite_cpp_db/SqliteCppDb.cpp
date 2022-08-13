//
// Created by bobini on 11.07.2022.
//

#include "SqliteCppDb.h"

thread_local std::map<std::string, SQLite::Database>
  db::sqlite_cpp_db::SqliteCppDb::connections;

db::sqlite_cpp_db::SqliteCppDb::SqliteCppDb(const std::string& dbPath)
{
    if (!connections.contains(dbPath)) {
        connections.emplace(
          dbPath,
          SQLite::Database(dbPath,
                           SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE));
        connections.at(dbPath).exec("PRAGMA journal_mode=WAL;");
        connections.at(dbPath).exec("PRAGMA synchronous=NORMAL;");
#ifdef DEBUG
        connections.at(dbPath).exec("PRAGMA foreign_keys=ON;");
#endif
    }
    connKey = dbPath;
}
auto
db::sqlite_cpp_db::SqliteCppDb::hasTable(const std::string& table) const -> bool
{
    return connections.at(connKey).tableExists(table);
}
auto
db::sqlite_cpp_db::SqliteCppDb::execute(const std::string& query) const -> void
{
    connections.at(connKey).exec(query);
}
