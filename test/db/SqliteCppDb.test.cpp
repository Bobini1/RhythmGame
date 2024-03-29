//
// Created by satou on 13.08.22.
//
#include <catch2/catch_test_macros.hpp>
#include "db/SqliteCppDb.h"
#include <filesystem>
#include <thread>

auto
getDb(const std::string& path) -> db::SqliteCppDb
{
    if (std::filesystem::exists(path)) {
        std::filesystem::remove(path);
    }
    return db::SqliteCppDb{ path };
}

TEST_CASE("Values can be inserted and retrieved from tables", "[SqliteCppDb]")
{
    using namespace std::literals::string_literals;
    auto db = getDb("test.db"s);
    REQUIRE_FALSE(db.hasTable("Test"));
    db.execute("CREATE TABLE Test(ID int, Name VARCHAR(255))"s);
    REQUIRE(db.hasTable("Test"));
    db.execute("INSERT INTO Test VALUES (1, 'TestName')"s);
    auto stmt = db.createStatement("SELECT * FROM Test WHERE ID = 1"s);
    auto row = stmt.executeAndGet<std::tuple<int, std::string>>();
    auto& [x, y] = row.value();
    REQUIRE(x == 1);
    REQUIRE(y == "TestName"s);
    db.execute("INSERT INTO Test VALUES (2, 'SecondRowName')"s);
    db.execute("INSERT INTO Test VALUES (69, 'ThirdRowName')"s);
    stmt = db.createStatement("SELECT * FROM Test"s);
    auto rows = stmt.executeAndGetAll<std::tuple<int, std::string>>();
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
    auto stmt = db.createStatement("SELECT * FROM Test WHERE ID = 1"s);
    auto row = stmt.executeAndGet<std::tuple<int, std::string>>();
    REQUIRE_FALSE(row);
    stmt = db.createStatement("SELECT * FROM Test"s);
    auto rows = stmt.executeAndGetAll<std::tuple<int, std::string>>();
    REQUIRE(rows.empty());
}

TEST_CASE("Database wrapper can be passed to another thread", "[SqliteCppDb]")
{
    using namespace std::literals::string_literals;
    auto db = getDb("test3.db"s);
    REQUIRE_FALSE(db.hasTable("Test"));
    db.execute("CREATE TABLE Test(ID int, Name VARCHAR(255))"s);
    db.execute("INSERT INTO Test VALUES (1, 'TestName')"s);
    auto stmt = db.createStatement("SELECT * FROM Test WHERE ID = 1"s);
    auto row = stmt.executeAndGet<std::tuple<int, std::string>>();
    auto& [x, y] = row.value();
    REQUIRE(x == 1);
    REQUIRE(y == "TestName"s);
    auto thread = std::thread{ [&db]() {
        auto stmt = db.createStatement("SELECT * FROM Test WHERE ID = 1"s);
        auto row = stmt.executeAndGet<std::tuple<int, std::string>>();
        auto& [z, w] = row.value();
        REQUIRE(z == 1);
        REQUIRE(w == "TestName"s);
    } };
    thread.join();
}

TEST_CASE("Values can be inserted into custom aggregate structs",
          "[SqliteCppDb]")
{
    using namespace std::string_literals;
    struct TestStruct
    {
        int x;
        std::string y;
    };
    static_assert(std::is_aggregate_v<TestStruct>);
    auto db = getDb("test4.db"s);
    REQUIRE_FALSE(db.hasTable("Test"));
    db.execute("CREATE TABLE Test(ID int, Name VARCHAR(255))"s);
    db.execute("INSERT INTO Test VALUES (1, 'TestName')"s);
    auto stmt = db.createStatement("SELECT * FROM Test WHERE ID = 1"s);
    auto row = stmt.executeAndGet<TestStruct>();
    REQUIRE(row);
    auto& [x, y] = row.value();
    REQUIRE(x == 1);
    REQUIRE(y == "TestName"s);

    stmt = db.createStatement("SELECT * FROM Test"s);
    auto rows = stmt.executeAndGetAll<TestStruct>();
    REQUIRE(rows.size() == 1);
    row = rows[0];
    REQUIRE(x == 1);
    REQUIRE(y == "TestName"s);
}

TEST_CASE("Simple scalar types don't need to be wrapped in structs or tuples",
          "[SqliteCppDb]")
{
    using namespace std::string_literals;
    auto db = getDb("test5.db"s);
    REQUIRE_FALSE(db.hasTable("Test"));
    db.execute("CREATE TABLE Test(ID int, Name VARCHAR(255))"s);
    db.execute("INSERT INTO Test VALUES (1, 'TestName')"s);
    auto stmt = db.createStatement("SELECT ID FROM Test WHERE ID = 1"s);
    auto row = stmt.executeAndGet<int>();
    REQUIRE(row);
    auto& x = row.value();
    REQUIRE(x == 1);

    stmt = db.createStatement("SELECT ID FROM Test"s);
    auto rows = stmt.executeAndGetAll<int>();
    REQUIRE(rows.size() == 1);
    row = rows[0];
    REQUIRE(x == 1);
}