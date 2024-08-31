//
// Created by bobini on 11.07.2022.
//

#include "SqliteCppDb.h"
#include "sqlite3.h"

#include <utility>

db::SqliteCppDb::
SqliteCppDb(std::filesystem::path dbPath)
  : db(dbPath,
       SQLite::OPEN_READWRITE | // NOLINT(hicpp-signed-bitwise)
         SQLite::OPEN_CREATE)
{
    db.exec("PRAGMA journal_mode=WAL;");
    db.exec("PRAGMA synchronous=NORMAL;");
    db.exec("PRAGMA optimize=0x10002;");
    sqlite3_limit(db.getHandle(), SQLITE_LIMIT_WORKER_THREADS, std::thread::hardware_concurrency());
#ifdef DEBUG
    db.exec("PRAGMA foreign_keys=ON;");
#endif
}

auto
db::SqliteCppDb::hasTable(const std::string& table) const -> bool
{
    return db.tableExists(table);
}
auto
db::SqliteCppDb::execute(const std::string& query) -> int64_t
{
    std::lock_guard lock(dbMutex);
    db.exec(query);
    return db.getLastInsertRowid();
}
auto
db::SqliteCppDb::createStatement(const std::string& query) -> Statement
{
    std::lock_guard lock(dbMutex);
    return Statement{ SQLite::Statement(db, query), &dbMutex, &db };
}
auto
db::SqliteCppDb::Statement::execute() -> int64_t
{
    std::lock_guard lock(*dbMutex);
    statement.exec();
    return db->getLastInsertRowid();
}
void
db::SqliteCppDb::Statement::reset()
{
    std::lock_guard lock(*dbMutex);
    statement.reset();
    statement.clearBindings();
}
db::SqliteCppDb::Statement::
Statement(SQLite::Statement statement,
          std::mutex* dbMutex,
          SQLite::Database* db)
  : statement(std::move(statement))
  , dbMutex(dbMutex)
  , db(db)
{
}
