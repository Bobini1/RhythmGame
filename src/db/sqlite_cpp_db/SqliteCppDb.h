//
// Created by bobini on 11.07.2022.
//

#ifndef RHYTHMGAME_SQLITECPPDB_H
#define RHYTHMGAME_SQLITECPPDB_H
#include "../Db.h"

#include <iostream>
namespace db::sql_db {

class SqliteCppDb : public Db
{
  public:
    ~SqliteCppDb() override = default;
    auto insert(const std::string& key, const std::string& value)
      -> void override;
    auto get(const std::string& key) -> std::string override;
};
} // namespace db::sqlite_cpp_db

#endif // RHYTHMGAME_SQLITECPPDB_H
