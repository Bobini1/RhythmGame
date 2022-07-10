//
// Created by PC on 05/07/2022.
//

#include <iostream>
#include <inttypes.h>

#include "DB.h"
#include <SQLiteCpp/SQLiteCpp.h>
#include <SQLiteCpp/VariadicBind.h>
namespace rhythm_game::db {

class SQLiteDb : public Db
{
  public:
    ~SQLiteDb() override = default;
    auto insert(const std::string& key, const std::string& value)
      -> void override
    {
        std::cout << "SQLiteDb::insert()" << std::endl;
    }
    auto get(const std::string& key) -> std::string override
    {
        std::cout << "SQLiteDb::get()" << std::endl;
        return "";
    }
};

auto
create_db() -> std::unique_ptr<Db>
{
    return std::make_unique<SQLiteDb>();
}
} // namespace src::db
