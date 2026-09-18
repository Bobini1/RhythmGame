//
// Created by bobini on 11.07.2022.
//

#ifndef RHYTHMGAME_SQLITECPPDB_H
#define RHYTHMGAME_SQLITECPPDB_H
#include <SQLiteCpp/SQLiteCpp.h>
#include <optional>
#include <vector>
#include <type_traits>
#include <chrono>
#include <cstdint>
#include "support/get.h"
#include "support/TupleSize.h"

struct sqlite3_mutex;

/**
 * @brief Namespace for database related classes and functions.
 */
namespace db {

/**
 * @brief Database wrapper for SQLiteCpp.
 * @note Independent statements may share a connection across threads. Use a
 * transaction to isolate a sequence of operations on that connection.
 */
class SqliteCppDb
{
    SQLite::Database db;
    uint64_t nextSavepoint = 0;
    unsigned transactionDepth = 0;

    void checkTransaction() const;

    class ConnectionLock
    {
        sqlite3_mutex* mutex;

      public:
        explicit ConnectionLock(const SQLite::Database& database);
        ~ConnectionLock();
        ConnectionLock(const ConnectionLock&) = delete;
        auto operator=(const ConnectionLock&) -> ConnectionLock& = delete;
        void unlock();
    };

    class StatementExecution
    {
        ConnectionLock lock;
        SQLite::Statement& statement;
        bool finished = false;

      public:
        StatementExecution(SQLite::Statement& statement,
                           const SqliteCppDb& database);
        ~StatementExecution();
        void finish();
    };

  public:
    /**
     * @brief Isolates a transaction from other users of this connection.
     * @details Nested transactions use savepoints. Uncommitted changes are
     * rolled back on destruction. Keep transactions on their creating thread
     * and do not wait for other threads or dispatch callbacks while holding one.
     */
    class Transaction
    {
        ConnectionLock lock;
        SqliteCppDb& owner;
        SQLite::Database& database;
        std::string commitQuery;
        std::string rollbackQuery;
        bool nested;
        bool committed = false;

      public:
        explicit Transaction(SqliteCppDb& database);
        ~Transaction();
        void commit();
    };

    [[nodiscard]] auto transaction() -> Transaction;

    /**
     * @brief Wrapper for SQLiteCpp::Statement.
     * @note A statement must not be used by multiple threads simultaneously.
     */
    class Statement
    {
        SQLite::Statement statement;
        SqliteCppDb* db;

      public:
        Statement(SQLite::Statement statement, SqliteCppDb* db);
        template<typename... T>
        auto bind(int index, T&&... values) -> void
        {
            const ConnectionLock lock(db->db);
            statement.bind(index, std::forward<T>(values)...);
        }
        template<typename... T>
        auto bind(const std::string& name, T&&... values) -> void
        {
            const ConnectionLock lock(db->db);
            statement.bind(name, std::forward<T>(values)...);
        }
        void reset();

        void execute();

        /**
         * @brief Executes a query that returns a single row.
         * @return An optional holding the result of the query. It will be empty
         * if the query didn't return anything. The cursor is reset after copying
         * the row, including on failure. Bindings are retained. Too few columns
         * for the requested result type cause an exception.
         * @tparam Ret The type that the result will be stored in can be a tuple
         * or an aggregate. Must be default constructible.
         */
        template<std::default_initializable Ret>
        [[nodiscard]] auto executeAndGet() -> std::optional<Ret>
        {
            StatementExecution execution(statement, *db);
            std::optional<Ret> result;
            if (statement.executeStep()) {
                result.emplace();
                writeRow(statement, *result);
            }
            execution.finish();
            return result;
        }
        /**
         * @brief Executes a query that returns any number of rows.
         * @return A vector holding the result of the query. It will be empty if
         * the query didn't return anything. If the query returns fewer columns
         * than specified in the template parameters, this method will throw.
         * @tparam Ret The type that the result will be stored in can be a tuple
         * or an aggregate. Must be default constructible.
         */
        template<std::default_initializable Ret>
        [[nodiscard]] auto executeAndGetAll() -> std::vector<Ret>
        {
            StatementExecution execution(statement, *db);
            std::vector<Ret> result;

            while (statement.executeStep()) {
                result.emplace_back();
                writeRow(statement, result.back());
            }

            execution.finish();
            return result;
        }

      private:
        template<typename ElemType>
        static auto getElem(SQLite::Statement& statement, int index) -> ElemType
        {
            // This if is necessary for MSVC (don't ask me why)
            if constexpr (std::is_same_v<ElemType, std::string>) {
                return statement.getColumn(index).getString();
            } else {
                return static_cast<ElemType>(statement.getColumn(index));
            }
        }

        template<std::default_initializable Ret>
        void writeRow(SQLite::Statement& statement, Ret& ret) const
        {
            int index = 0;
            writeRow(statement, ret, index);
        }

        template<std::default_initializable Ret>
        void writeRow(SQLite::Statement& stmt, Ret& ret, int& index) const
        {
            if constexpr (std::convertible_to<SQLite::Column, Ret> || std::is_same_v<Ret, std::string>) {
                ret = getElem<std::remove_cvref_t<Ret>>(stmt, index++);
            } else {
                constexpr size_t tupleSize = support::tupleSizeV<Ret>;
                constexpr auto indices =
                  std::make_integer_sequence<int, static_cast<int>(tupleSize)>();
                [this, &stmt, &ret, &index]<int... N>(std::integer_sequence<int, N...>) {
                    (writeRow(stmt, support::get<N>(ret), index), ...);
                }(indices);
            }
        }
    };

    /**
     * @brief Constructs a database wrapper.
     * @param dbPath Path to the database file.
     * The database file will be created if it does not exist.
     * @param busyTimeout How long to wait for locks held by other connections.
     * Shared-connection transactions are serialized independently of this timeout.
     */
    explicit SqliteCppDb(
      const std::filesystem::path& dbPath,
      std::chrono::milliseconds busyTimeout = std::chrono::milliseconds{ 0 });
    /**
     * @brief Executes a query.
     * @note Good for single-use queries. Use Statement otherwise.
     * @param query Query to execute.
     */
    void execute(const std::string& query);
    auto createStatement(const std::string& query) -> Statement;
    /**
     * @brief Queries the database to inspect whether the table with the
     * provided name exists.
     * @param table Name of the table.
     * @return True if the table exists, false otherwise.
     */
    [[nodiscard]] auto hasTable(const std::string& table) const -> bool;
};
} // namespace db

#endif // RHYTHMGAME_SQLITECPPDB_H
