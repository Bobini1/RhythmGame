#include <iostream>
#include <memory>
#include <sol/sol.hpp>
#include "db/sqlite_cpp_db/SqliteCppDb.h"

auto
main() -> int
{
    sol::state lua;
    // remember to add sol::lib::jit to use LuaJIT
    lua.open_libraries(sol::lib::base, sol::lib::jit);
    auto gameDb = std::make_unique<db::sqlite_cpp_db::SqliteCppDb>();
    gameDb->insert("key", "value");
    std::cout << gameDb->get("key") << '\n';
    lua.script(R"(print(jit.version)
print(_VERSION))");
}
