//
// Created by bobini on 11.07.2022.
//

#include "SqliteCppDb.h"
#include "sqlite3.h"

#include <utility>
#include <thread>
#include <stdexcept>
#include <spdlog/spdlog.h>

db::SqliteCppDb::SqliteCppDb(const std::filesystem::path& dbPath,
                             std::chrono::milliseconds busyTimeout,
                             Durability durability)
  : SqliteCppDb(dbPath, busyTimeout, durability, false)
{
}

auto
db::SqliteCppDb::openReadOnly(const std::filesystem::path& dbPath,
                              std::chrono::milliseconds busyTimeout)
  -> SqliteCppDb
{
    return SqliteCppDb(dbPath, busyTimeout, Durability::Normal, true);
}

db::SqliteCppDb::SqliteCppDb(const std::filesystem::path& dbPath,
                             std::chrono::milliseconds busyTimeout,
                             Durability durability,
                             bool readOnly)
  : db(dbPath,
       (readOnly ? SQLite::OPEN_READONLY
                 : SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE) |
         SQLite::OPEN_FULLMUTEX)
{
    db.setBusyTimeout(static_cast<int>(busyTimeout.count()));
    if (readOnly) {
        db.exec("PRAGMA query_only=ON;");
    } else {
        db.exec("PRAGMA journal_mode=WAL;");
        db.exec(durability == Durability::Full ? "PRAGMA synchronous=FULL;"
                                               : "PRAGMA synchronous=NORMAL;");
        db.exec("PRAGMA optimize=0x10002;");
    }
    sqlite3_limit(db.getHandle(),
                  SQLITE_LIMIT_WORKER_THREADS,
                  std::thread::hardware_concurrency());
    db.exec("PRAGMA foreign_keys=ON;");
}

auto
db::SqliteCppDb::hasTable(const std::string& table) const -> bool
{
    const ConnectionLock lock(&db);
    checkTransaction();
    return db.tableExists(table);
}
void
db::SqliteCppDb::execute(const std::string& query)
{
    const ConnectionLock lock(&db);
    checkTransaction();
    db.exec(query);
}
auto
db::SqliteCppDb::createStatement(const std::string& query) -> Statement
{
    const ConnectionLock lock(&db);
    checkTransaction();
    return Statement{ SQLite::Statement(db, query), this };
}
void
db::SqliteCppDb::Statement::execute(std::stop_token stopToken)
{
    StatementExecution execution(&statement, db, stopToken);
    statement.exec();
    execution.finish();
}
void
db::SqliteCppDb::Statement::reset()
{
    const ConnectionLock lock(&db->db);
    statement.reset();
    statement.clearBindings();
}
db::SqliteCppDb::Statement::Statement(SQLite::Statement statement,
                                      SqliteCppDb* db)
  : statement(std::move(statement))
  , db(db)
{
}

db::SqliteCppDb::ConnectionLock::ConnectionLock(
  const SQLite::Database* database)
  : mutex(sqlite3_db_mutex(database->getHandle()))
{
    sqlite3_mutex_enter(mutex);
}

db::SqliteCppDb::ConnectionLock::~ConnectionLock()
{
    unlock();
}

void
db::SqliteCppDb::ConnectionLock::unlock()
{
    sqlite3_mutex_leave(std::exchange(mutex, nullptr));
}

db::SqliteCppDb::StatementExecution::StatementExecution(
  SQLite::Statement* statement,
  const SqliteCppDb* owner,
  std::stop_token stopToken)
  : lock(&owner->db)
  , statement(statement)
  , database(&owner->db)
  , stopToken(stopToken)
{
    owner->checkTransaction();
    checkCancelled();
    if (stopToken.stop_possible()) {
        // The connection lock confines this handler to the current execution.
        sqlite3_progress_handler(
          database->getHandle(),
          1000,
          [](void* context) {
              return static_cast<StatementExecution*>(context)
                         ->stopToken.stop_requested()
                       ? 1
                       : 0;
          },
          this);
    }
}

db::SqliteCppDb::StatementExecution::~StatementExecution()
{
    if (stopToken.stop_possible()) {
        sqlite3_progress_handler(database->getHandle(), 0, nullptr, nullptr);
    }
    if (!finished) {
        // Preserve the original exception while releasing the cursor.
        statement->tryReset();
    }
}

void
db::SqliteCppDb::StatementExecution::checkCancelled() const
{
    if (stopToken.stop_requested()) {
        throw SQLite::Exception("Database operation cancelled",
                                SQLITE_INTERRUPT);
    }
}

void
db::SqliteCppDb::StatementExecution::finish()
{
    // Reset can report a commit failure for INSERT ... RETURNING.
    statement->reset();
    finished = true;
}

db::SqliteCppDb::Transaction::Transaction(SqliteCppDb* owner)
  : lock(&owner->db)
  , owner(owner)
  , database(&owner->db)
  , nested(owner->transactionDepth != 0)
  , depth(owner->transactionDepth + 1)
{
    owner->checkTransaction();
    if (nested) {
        const auto name =
          "rhythmgame_" + std::to_string(++owner->nextSavepoint);
        commitQuery = "RELEASE SAVEPOINT " + name;
        rollbackQuery = "ROLLBACK TO SAVEPOINT " + name;
        database->exec("SAVEPOINT " + name);
    } else {
        commitQuery = "COMMIT";
        rollbackQuery = "ROLLBACK";
        database->exec("BEGIN");
    }
    ++owner->transactionDepth;
}

db::SqliteCppDb::Transaction::~Transaction()
{
    if (committed) {
        return;
    }
    --owner->transactionDepth;
    if (sqlite3_get_autocommit(database->getHandle()) != 0) {
        return;
    }
    auto result = database->tryExec(rollbackQuery);
    if (result == SQLITE_OK && nested) {
        result = database->tryExec(commitQuery);
    }
    if (result != SQLITE_OK) {
        // If a savepoint cannot be restored, abort the enclosing transaction.
        // Its subsequent commit must fail rather than publish partial changes.
        database->tryExec("ROLLBACK");
        spdlog::error("Failed to roll back database transaction: {}", result);
    }
}

void
db::SqliteCppDb::Transaction::commit()
{
    if (committed) {
        throw std::logic_error("Transaction has already been committed");
    }
    if (depth != owner->transactionDepth) {
        throw std::logic_error(
          "Finish the inner transaction before committing its parent");
    }
    owner->checkTransaction();
    database->exec(commitQuery);
    --owner->transactionDepth;
    committed = true;
    lock.unlock();
}

auto
db::SqliteCppDb::transaction() -> Transaction
{
    return Transaction(this);
}

void
db::SqliteCppDb::checkTransaction() const
{
    const auto active = sqlite3_get_autocommit(db.getHandle()) == 0;
    if (active != (transactionDepth != 0)) {
        throw std::runtime_error("Database transaction was aborted; leave its "
                                 "scope before reusing the connection");
    }
}
