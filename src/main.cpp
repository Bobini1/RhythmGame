#include <iostream>
#include <memory>

#include "db/sqlite_cpp_db/SqliteCppDb.h"

auto
main() -> int
{
    auto gameDb = std::make_unique<db::sql_db::SqliteCppDb>();
    gameDb->insert("key", "value");
    std::cout << gameDb->get("key") << std::endl;
}
