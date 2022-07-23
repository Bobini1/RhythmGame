//
// Created by bobini on 11.07.2022.
//

#include "SqliteCppDb.h"

#include <SQLiteCpp/SQLiteCpp.h>
#include <SQLiteCpp/VariadicBind.h>

#include <memory>

thread_local std::unique_ptr<SQLite::Database>
  db::sqlite_cpp_db::SqliteCppDb::db;

db::sqlite_cpp_db::SqliteCppDb::SqliteCppDb(const std::string& dbPath)
{
    if (!db) {
        db = std::make_unique<SQLite::Database>(
          dbPath, SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);
        db->exec("PRAGMA journal_mode=WAL;");
        db->exec("PRAGMA synchronous=NORMAL;");
        db->exec("PRAGMA foreign_keys=ON;");
    }
}
auto
db::sqlite_cpp_db::SqliteCppDb::hasTable(const std::string& table) const -> bool
{
    return db->tableExists(table);
}
auto
db::sqlite_cpp_db::SqliteCppDb::execute(const std::string& query) const -> void
{
    db->exec(query);
}
auto
db::sqlite_cpp_db::SqliteCppDb::executeAndGet(const std::string& query) const
  -> std::optional<std::any>
{
    return std::nullopt;
}
auto
db::sqlite_cpp_db::SqliteCppDb::executeAndGetAll(const std::string& query) const
  -> std::vector<std::any>
{
    return {};
}
