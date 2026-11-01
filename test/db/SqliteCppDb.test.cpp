//
// Created by satou on 13.08.22.
//
#include <catch2/catch_test_macros.hpp>
#include "db/SqliteCppDb.h"
#include <filesystem>
#include <thread>
#include <future>
#include <QTemporaryDir>
#include <sqlite3.h>
#include "support/QStringToPath.h"

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
    REQUIRE_FALSE(row.has_value());
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

TEST_CASE("Concurrent inserts return their own generated IDs", "[SqliteCppDb]")
{
    db::SqliteCppDb database(":memory:");
    database.execute(
      "CREATE TABLE entries (id INTEGER PRIMARY KEY, value TEXT)");
    using Inserted = std::pair<int64_t, std::string>;
    std::vector<std::future<std::vector<Inserted>>> workers;
    for (int worker = 0; worker < 4; ++worker) {
        workers.push_back(std::async(std::launch::async, [&, worker] {
            auto insert = database.createStatement(
              "INSERT INTO entries (value) VALUES (?) RETURNING id");
            std::vector<Inserted> inserted;
            for (int index = 0; index < 50; ++index) {
                const auto value =
                  std::to_string(worker) + ":" + std::to_string(index);
                insert.bind(1, value);
                inserted.emplace_back(insert.executeAndGet<int64_t>().value(),
                                      value);
            }
            return inserted;
        }));
    }
    for (auto& worker : workers) {
        for (const auto& [id, value] : worker.get()) {
            auto query = database.createStatement(
              "SELECT value FROM entries WHERE id = ?");
            query.bind(1, id);
            CHECK(query.executeAndGet<std::string>() == value);
        }
    }
}

TEST_CASE("A rollback cannot include another thread's save", "[SqliteCppDb]")
{
    db::SqliteCppDb database(":memory:");
    database.execute("CREATE TABLE entries (value TEXT)");
    std::promise<void> attempting;
    auto started = attempting.get_future();
    std::future<int> other;
    bool blocked = false;
    {
        auto transaction = database.transaction();
        database.execute("INSERT INTO entries VALUES ('rolled back')");
        other = std::async(std::launch::async, [&] {
            attempting.set_value();
            const auto count =
              database.createStatement("SELECT count(*) FROM entries")
                .executeAndGet<int>()
                .value();
            database.execute("INSERT INTO entries VALUES ('other thread')");
            return count;
        });
        started.wait();
        blocked = other.wait_for(std::chrono::milliseconds(30)) ==
                  std::future_status::timeout;
    }
    CHECK(other.get() == 0);
    CHECK(blocked);
    CHECK(database.createStatement("SELECT value FROM entries")
            .executeAndGetAll<std::string>() ==
          std::vector<std::string>{ "other thread" });
}

TEST_CASE("Nested transactions roll back only their own changes",
          "[SqliteCppDb]")
{
    db::SqliteCppDb database(":memory:");
    database.execute("CREATE TABLE entries (value TEXT)");
    auto outer = database.transaction();
    database.execute("INSERT INTO entries VALUES ('first')");
    {
        auto inner = database.transaction();
        database.execute("INSERT INTO entries VALUES ('discarded')");
    }
    {
        auto inner = database.transaction();
        database.execute("INSERT INTO entries VALUES ('last')");
        inner.commit();
    }
    outer.commit();
    // commit releases the connection even while the scope object is alive.
    auto query = std::async(std::launch::async, [&] {
        return database
          .createStatement("SELECT value FROM entries ORDER BY rowid")
          .executeAndGetAll<std::string>();
    });
    CHECK(query.get() == std::vector<std::string>{ "first", "last" });
}

TEST_CASE("An outer transaction cannot commit while a nested scope is active",
          "[SqliteCppDb]")
{
    db::SqliteCppDb database(":memory:");
    database.execute("CREATE TABLE entries (value TEXT)");
    {
        auto outer = database.transaction();
        database.execute("INSERT INTO entries VALUES ('outer')");
        {
            auto inner = database.transaction();
            database.execute("INSERT INTO entries VALUES ('inner')");
            REQUIRE_THROWS_AS(outer.commit(), std::logic_error);
            inner.commit();
        }
        // Releasing the inner savepoint still leaves its changes provisional.
    }
    CHECK(database.createStatement("SELECT count(*) FROM entries")
            .executeAndGet<int>() == 0);
}

TEST_CASE("Failed commits are rolled back before reusing the connection",
          "[SqliteCppDb]")
{
    db::SqliteCppDb database(":memory:");
    database.execute("CREATE TABLE parent (id INTEGER PRIMARY KEY)");
    database.execute(
      "CREATE TABLE child (parent_id INTEGER REFERENCES parent(id) "
      "DEFERRABLE INITIALLY DEFERRED)");
    {
        auto transaction = database.transaction();
        database.execute("INSERT INTO child VALUES (1)");
        REQUIRE_THROWS_AS(transaction.commit(), SQLite::Exception);
    }
    database.execute("INSERT INTO parent VALUES (1)");
    CHECK(database.createStatement("SELECT count(*) FROM child")
            .executeAndGet<int>() == 0);
    CHECK(database.createStatement("SELECT count(*) FROM parent")
            .executeAndGet<int>() == 1);
}

TEST_CASE(
  "An automatic rollback cannot silently turn a batch into autocommit writes",
  "[SqliteCppDb]")
{
    db::SqliteCppDb database(":memory:");
    database.execute("CREATE TABLE entries (value TEXT)");
    database.execute("CREATE TRIGGER abort_batch BEFORE INSERT ON entries "
                     "WHEN NEW.value = 'abort' BEGIN SELECT RAISE(ROLLBACK, "
                     "'injected rollback'); END");
    auto retained =
      database.createStatement("INSERT INTO entries VALUES ('later')");
    {
        auto outer = database.transaction();
        database.execute("INSERT INTO entries VALUES ('first')");
        {
            auto inner = database.transaction();
            REQUIRE_THROWS_AS(
              database.execute("INSERT INTO entries VALUES ('abort')"),
              SQLite::Exception);
        }
        REQUIRE_THROWS(retained.execute());
        REQUIRE_THROWS(
          database.execute("INSERT INTO entries VALUES ('later')"));
        REQUIRE_THROWS(database.transaction());
        REQUIRE_THROWS(outer.commit());
    }
    CHECK(database.createStatement("SELECT count(*) FROM entries")
            .executeAndGet<int>() == 0);
    REQUIRE_NOTHROW(retained.execute());
}

TEST_CASE("Single-row queries release WAL snapshots even when decoding fails",
          "[SqliteCppDb]")
{
    QTemporaryDir directory;
    REQUIRE(directory.isValid());
    const auto path =
      support::qStringToPath(directory.filePath("snapshots.sqlite"));
    db::SqliteCppDb first(path);
    first.execute("CREATE TABLE entries (id INTEGER PRIMARY KEY)");
    first.execute("INSERT INTO entries VALUES (1), (2)");
    db::SqliteCppDb second(path);
    auto retained = first.createStatement("SELECT id FROM entries");
    SECTION("Successful query")
    {
        REQUIRE(retained.executeAndGet<int>() == 1);
    }
    SECTION("Result conversion failure")
    {
        REQUIRE_THROWS(retained.executeAndGet<std::tuple<int, int>>());
    }
    second.execute("INSERT INTO entries VALUES (3)");
    REQUIRE_NOTHROW(first.execute("INSERT INTO entries VALUES (4)"));
    CHECK(first.createStatement("SELECT count(*) FROM entries")
            .executeAndGet<int>() == 4);
}

TEST_CASE("RETURNING reports commit failures before returning an ID",
          "[SqliteCppDb]")
{
    QTemporaryDir directory;
    REQUIRE(directory.isValid());
    const auto path =
      support::qStringToPath(directory.filePath("returning.sqlite"));
    db::SqliteCppDb writer(path);
    writer.execute("PRAGMA journal_mode=DELETE");
    writer.execute("CREATE TABLE entries (id INTEGER PRIMARY KEY)");
    writer.execute("INSERT INTO entries VALUES (1), (2)");
    SQLite::Database reader(path, SQLite::OPEN_READONLY);
    SQLite::Statement retained(reader, "SELECT id FROM entries");
    REQUIRE(retained.executeStep());
    auto insert =
      writer.createStatement("INSERT INTO entries DEFAULT VALUES RETURNING id");
    REQUIRE_THROWS_AS(insert.executeAndGet<int64_t>(), SQLite::Exception);
    retained.reset();
    CHECK(writer.createStatement("SELECT count(*) FROM entries")
            .executeAndGet<int>() == 2);
    CHECK(insert.executeAndGet<int64_t>() == 3);
}

TEST_CASE("Read connections see committed data without waiting for writers",
          "[SqliteCppDb][scheduling]")
{
    QTemporaryDir directory;
    REQUIRE(directory.isValid());
    const auto path =
      support::qStringToPath(directory.filePath("readers.sqlite"));
    db::SqliteCppDb writer(path);
    writer.execute("CREATE TABLE entries(value INTEGER)");
    writer.execute("INSERT INTO entries VALUES(1)");
    auto reader = db::SqliteCppDb::openReadOnly(path);
    auto read = reader.createStatement("SELECT value FROM entries");
    auto transaction = writer.transaction();
    writer.execute("UPDATE entries SET value = 2");
    CHECK(read.executeAndGet<int>() == 1);
    transaction.commit();
    CHECK(read.executeAndGet<int>() == 2);
    REQUIRE_THROWS_AS(reader.execute("UPDATE entries SET value = 3"),
                      SQLite::Exception);
    CHECK(read.executeAndGet<int>() == 2);
}

TEST_CASE(
  "Cancelled statements skip execution after waiting for the connection",
  "[SqliteCppDb][scheduling]")
{
    db::SqliteCppDb database(":memory:");
    database.execute("CREATE TABLE entries(value INTEGER)");
    auto insert = database.createStatement("INSERT INTO entries VALUES(1)");
    std::stop_source cancellation;
    std::promise<void> attempting;
    auto started = attempting.get_future();
    auto transaction = database.transaction();
    auto worker = std::async(std::launch::async, [&] {
        attempting.set_value();
        try {
            insert.execute(cancellation.get_token());
            return SQLITE_OK;
        } catch (const SQLite::Exception& error) {
            return error.getErrorCode();
        }
    });
    started.wait();
    cancellation.request_stop();
    transaction.commit();
    CHECK(worker.get() == SQLITE_INTERRUPT);
    CHECK(database.createStatement("SELECT count(*) FROM entries")
            .executeAndGet<int>() == 0);
    REQUIRE_NOTHROW(insert.execute());
}

TEST_CASE("Cancelling a running query leaves the connection reusable",
          "[SqliteCppDb][scheduling]")
{
    using namespace std::chrono_literals;
    db::SqliteCppDb database(":memory:");
    auto query = database.createStatement(
      "WITH RECURSIVE numbers(n) AS (VALUES(0) UNION ALL "
      "SELECT n+1 FROM numbers WHERE n < 100000000) SELECT sum(n) FROM "
      "numbers");
    std::stop_source cancellation;
    std::promise<void> attempting;
    auto started = attempting.get_future();
    auto worker = std::async(std::launch::async, [&] {
        attempting.set_value();
        try {
            (void)query.executeAndGet<int64_t>(cancellation.get_token());
            return SQLITE_OK;
        } catch (const SQLite::Exception& error) {
            return error.getErrorCode();
        }
    });
    started.wait();
    const auto wasRunning =
      worker.wait_for(20ms) == std::future_status::timeout;
    cancellation.request_stop();
    CHECK(wasRunning);
    CHECK(worker.wait_for(2s) == std::future_status::ready);
    CHECK(worker.get() == SQLITE_INTERRUPT);
    CHECK(database.createStatement("SELECT 42").executeAndGet<int>() == 42);
}
