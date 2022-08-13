//
// Created by bobini on 11.07.2022.
//

#include "SqliteCppDb.h"

thread_local std::unique_ptr<SQLite::Database>
  db::sqlite_cpp_db::SqliteCppDb::db;

db::sqlite_cpp_db::SqliteCppDb::SqliteCppDb(const std::string& dbPath)
{
    if (!db) {
        db = std::make_unique<SQLite::Database>(
          dbPath, SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);
        db->exec("PRAGMA journal_mode=WAL;");
        db->exec("PRAGMA synchronous=NORMAL;");
#ifdef DEBUG
        db->exec("PRAGMA foreign_keys=ON;");
#endif
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
