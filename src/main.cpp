#include <iostream>
#include "db/sqlite_cpp_db/SqliteCppDb.h"

auto
main() -> int
{
    auto db = db::sqlite_cpp_db::SqliteCppDb{ "db.db" };
    db.execute("INSERT INTO test VALUES(2, 'user')");
    auto res = db.executeAndGetAll<int, std::string>("SELECT * FROM test");
    for (auto& [index, name] : res)
        std::cout << index << " " << name << "\n";
}
