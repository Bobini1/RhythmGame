//
// Created by bobini on 11.07.2022.
//

#include "SqliteCppDb.h"

#include <utility>

thread_local db::DatabaseAccessPoint db::SqliteCppDb::connections;

db::SqliteCppDb::SqliteCppDb(std::string dbPath)
  : connKey(std::move(dbPath))
{
}

auto
db::SqliteCppDb::hasTable(const std::string& table) const -> bool
{
    return connections[connKey].tableExists(table);
}
auto
db::SqliteCppDb::execute(const std::string& query) const -> void
{
    connections[connKey].exec(query);
}
