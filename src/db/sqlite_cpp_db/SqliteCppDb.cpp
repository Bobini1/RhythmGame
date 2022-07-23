//
// Created by bobini on 11.07.2022.
//

#include "SqliteCppDb.h"

#include <SQLiteCpp/SQLiteCpp.h>
#include <SQLiteCpp/VariadicBind.h>
auto
db::sqlite_cpp_db::SqliteCppDb::get(const std::string& key) -> std::string
{
    std::cout << "SqliteCppDb::get()" << std::endl;
    return "";
}
auto
db::sqlite_cpp_db::SqliteCppDb::insert(const std::string& key,
                                       const std::string& value) -> void
{
    std::cout << "SqliteCppDb::insert()" << std::endl;
}
