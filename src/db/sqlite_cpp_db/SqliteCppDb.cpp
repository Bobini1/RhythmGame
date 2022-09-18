//
// Created by bobini on 11.07.2022.
//

#include "SqliteCppDb.h"

#include <utility>

thread_local DatabaseAccessPoint db::sqlite_cpp_db::SqliteCppDb::connections;

db::sqlite_cpp_db::SqliteCppDb::SqliteCppDb(std::string dbPath)
  : connKey(std::move(dbPath))
{
}

auto
db::sqlite_cpp_db::SqliteCppDb::hasTable(const std::string& table) const -> bool
{
    return connections[connKey].tableExists(table);
}
auto
db::sqlite_cpp_db::SqliteCppDb::execute(const std::string& query) const -> void
{
    connections[connKey].exec(query);
}
