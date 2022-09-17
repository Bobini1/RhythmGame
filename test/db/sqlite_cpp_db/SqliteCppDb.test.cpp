//
// Created by satou on 13.08.22.
//
#include <catch2/catch_test_macros.hpp>
#include "db/sqlite_cpp_db/SqliteCppDb.h"
#include <filesystem>
#include <thread>

db::sqlite_cpp_db::SqliteCppDb
getDb(const std::string& path)
{
    if (std::filesystem::exists(path)) {
        std::filesystem::remove(path);
    }
    return db::sqlite_cpp_db::SqliteCppDb{ path };
}

TEST_CASE("Values can be inserted and retrieved from tables", "[SqliteCppDb]")
{
    using namespace std::literals::string_literals;
    auto db = getDb("test.db"s);
    REQUIRE_FALSE(db.hasTable("Test"));
    db.execute("CREATE TABLE Test(ID int, Name VARCHAR(255))"s);
    REQUIRE(db.hasTable("Test"));
    db.execute("INSERT INTO Test VALUES (1, 'TestName')"s);
    auto row =
      db.executeAndGet<int, std::string>("SELECT * FROM Test WHERE ID = 1"s);
    auto& [x, y] = row.value();
    REQUIRE(x == 1);
    REQUIRE(y == "TestName"s);
    db.execute("INSERT INTO Test VALUES (2, 'SecondRowName')"s);
    db.execute("INSERT INTO Test VALUES (69, 'ThirdRowName')"s);
    auto rows = db.executeAndGetAll<int, std::string>("SELECT * FROM Test"s);
    row = rows[1];
    REQUIRE(x == 2);
    REQUIRE(y == "SecondRowName"s);
    row = rows[2];
    REQUIRE(x == 69);
    REQUIRE(y == "ThirdRowName"s);
}

TEST_CASE("Failing queries correctly return empty results", "[SqliteCppDb]")
{
    using namespace std::literals::string_literals;
    auto db = getDb("test2.db"s);
    REQUIRE_FALSE(db.hasTable("Test"));
    db.execute("CREATE TABLE Test(ID int, Name VARCHAR(255))"s);
    auto row =
      db.executeAndGet<int, std::string>("SELECT * FROM Test WHERE ID = 1"s);
    REQUIRE_FALSE(row);
    auto rows = db.executeAndGetAll<int, std::string>("SELECT * FROM Test"s);
    REQUIRE(rows.empty());
}

TEST_CASE("Database wrapper can be passed to another thread", "[SqliteCppDb]")
{
    using namespace std::literals::string_literals;
    auto db = getDb("test3.db"s);
    REQUIRE_FALSE(db.hasTable("Test"));
    db.execute("CREATE TABLE Test(ID int, Name VARCHAR(255))"s);
    db.execute("INSERT INTO Test VALUES (1, 'TestName')"s);
    auto row =
      db.executeAndGet<int, std::string>("SELECT * FROM Test WHERE ID = 1"s);
    auto& [x, y] = row.value();
    REQUIRE(x == 1);
    REQUIRE(y == "TestName"s);
    auto thread = std::thread{ [&db]() {
        auto row = db.executeAndGet<int, std::string>(
          "SELECT * FROM Test WHERE ID = 1"s);
        auto& [z, w] = row.value();
        REQUIRE(z == 1);
        REQUIRE(w == "TestName"s);
    } };
    thread.join();
}