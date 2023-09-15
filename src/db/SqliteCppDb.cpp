//
// Created by bobini on 11.07.2022.
//

#include "SqliteCppDb.h"
#include <sqlite3.h>

#include <utility>

db::SqliteCppDb::SqliteCppDb(std::string dbPath)
  : db(dbPath,
       SQLite::OPEN_READWRITE | // NOLINT(hicpp-signed-bitwise)
         SQLite::OPEN_CREATE)
{
    db.exec("PRAGMA journal_mode=WAL;");
    db.exec("PRAGMA synchronous=NORMAL;");
#ifdef DEBUG
    db.exec("PRAGMA foreign_keys=ON;");
#endif
    auto* handle = db.getHandle();
    sqlite3_busy_handler(
      handle, [](void*, int) { return 1; }, nullptr);
}

auto
db::SqliteCppDb::hasTable(const std::string& table) const -> bool
{
    return db.tableExists(table);
}
void
db::SqliteCppDb::execute(const std::string& query)
{
    std::lock_guard lock(dbMutex);
    db.exec(query);
}
auto
db::SqliteCppDb::createStatement(const std::string& query)
  -> db::SqliteCppDb::Statement
{
    std::lock_guard lock(dbMutex);
    return Statement{ SQLite::Statement(db, query), &dbMutex };
}
void
db::SqliteCppDb::Statement::execute()
{
    std::lock_guard lock(*dbMutex);
    statement.exec();
}
void
db::SqliteCppDb::Statement::reset()
{
    std::lock_guard lock(*dbMutex);
    statement.reset();
    statement.clearBindings();
}
db::SqliteCppDb::Statement::Statement(SQLite::Statement statement,
                                      std::mutex* dbMutex)
  : statement(std::move(statement))
  , dbMutex(dbMutex)
{
}
