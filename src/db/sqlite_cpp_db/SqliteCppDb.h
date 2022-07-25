//
// Created by bobini on 11.07.2022.
//

#ifndef RHYTHMGAME_SQLITECPPDB_H
#define RHYTHMGAME_SQLITECPPDB_H
#include "db/Db.h"
#include <SQLiteCpp/SQLiteCpp.h>

namespace db::sqlite_cpp_db {

class SqliteCppDb final : public Db
{
    thread_local static std::unique_ptr<SQLite::Database> db;

  public:
    explicit SqliteCppDb(const std::string& dbPath);
    [[nodiscard]] auto hasTable(const std::string& table) const
      -> bool override;
    auto execute(const std::string& query) const -> void override;
    [[nodiscard]] auto executeAndGet(const std::string& query) const
      -> std::optional<std::any> override;
    [[nodiscard]] auto executeAndGetAll(const std::string& query) const
      -> cppcoro::generator<std::any> override;
};
} // namespace db::sqlite_cpp_db

#endif // RHYTHMGAME_SQLITECPPDB_H
