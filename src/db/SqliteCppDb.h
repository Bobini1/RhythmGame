//
// Created by bobini on 11.07.2022.
//

#ifndef RHYTHMGAME_SQLITECPPDB_H
#define RHYTHMGAME_SQLITECPPDB_H
#include <SQLiteCpp/SQLiteCpp.h>
#include <optional>
#include <vector>
#include <map>
#include <type_traits>
#include "support/get.h"
#include "support/TupleSize.h"

namespace db {

/**
 * @brief Database wrapper for SQLiteCpp.
 * @note This class is thread safe.
 */
class SqliteCppDb
{
    SQLite::Database db;
    std::mutex dbMutex;

  public:
    /**
     * @brief Wrapper for SQLiteCpp::Statement.
     * @note This class is thread safe.
     */
    class Statement
    {
        SQLite::Statement statement;
        std::mutex* dbMutex;
        SQLite::Database* db;

      public:
        Statement(SQLite::Statement statement,
                  std::mutex* dbMutex,
                  SQLite::Database* db);
        template<typename... T>
        auto bind(int index, T&&... values) -> void
        {
            std::lock_guard lock(*dbMutex);
            statement.bind(index, std::forward<T>(values)...);
        }
        template<typename... T>
        auto bind(const std::string& name, T&&... values) -> void
        {
            std::lock_guard lock(*dbMutex);
            statement.bind(name, std::forward<T>(values)...);
        }
        void reset();

        auto execute() -> int64_t;

        /**
         * @brief Executes a query that returns a single row.
         * @return An optional holding the result of the query. It will be empty
         * if the query didn't return anything. If the query returns fewer
         * columns than specified in template parameters, they will be default
         * initialized.
         * @tparam Ret The type that the result will be stored in can be a tuple
         * or an aggregate. Must be default constructible.
         */
        template<std::default_initializable Ret>
        [[nodiscard]] auto executeAndGet() -> std::optional<Ret>
            requires(!std::convertible_to<SQLite::Column, Ret> &&
                     !std::is_same_v<Ret, std::string>)
        {
            std::lock_guard lock(*dbMutex);
            if (!statement.executeStep()) {
                return {};
            }
            Ret result{};
            writeRow(statement, result);

            return { std::move(result) };
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
            requires(!std::convertible_to<SQLite::Column, Ret> &&
                     !std::is_same_v<Ret, std::string>)
        {
            std::lock_guard lock(*dbMutex);
            std::vector<Ret> result;

            while (statement.executeStep()) {
                result.emplace_back();
                writeRow(statement, result.back());
            }

            return result;
        }

        /**
         * @brief Executes a query that returns a single value in a single
         * column.
         * @tparam Ret Type of the value to return.
         * @param query Query to execute.
         * @return An optional holding the result of the query. It will be empty
         * if the query didn't return anything.
         */
        template<typename Ret>
        auto executeAndGet() -> std::optional<Ret>
            requires std::convertible_to<SQLite::Column, Ret> ||
              std::is_same_v<Ret, std::string>
        {

            std::lock_guard lock(*dbMutex);
            if (!statement.executeStep()) {
                return {};
            }
            return { getElem<Ret>(statement, 0) };
        }

        /**
         * @brief Executes a query that returns any number of values in a single
         * column.
         * @tparam Ret Type of the values to return.
         * @param query Query to execute.
         * @return A vector holding the result of the query. It will be empty if
         * the query didn't return anything.
         */
        template<typename Ret>
        auto executeAndGetAll() -> std::vector<Ret>
            requires std::convertible_to<SQLite::Column, Ret> ||
              std::is_same_v<Ret, std::string>
        {
            std::lock_guard lock(*dbMutex);
            std::vector<Ret> result;

            while (statement.executeStep()) {
                result.emplace_back(getElem<Ret>(statement, 0));
            }

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
        auto writeRow(SQLite::Statement& statement, Ret& ret) const -> void
        {
            constexpr size_t tupleSize = support::tupleSizeV<Ret>;
            constexpr auto indices =
              std::make_integer_sequence<int, static_cast<int>(tupleSize)>();

            [&statement, &ret]<int... N>(std::integer_sequence<int, N...>) {
                ((support::get<N>(ret) = getElem<
                    std::remove_cvref_t<decltype(support::get<N>(ret))>>(
                    statement, N)),
                 ...);
            }(indices);
        }
    };

    /**
     * @brief Constructs a database wrapper.
     * @param dbPath Path to the database file.
     * The database file will be created if it does not exist.
     */
    explicit SqliteCppDb(std::string dbPath);
    /**
     * @brief Executes a query.
     * @note Good for single-use queries. Use Statement otherwise.
     * @param query Query to execute.
     * @return The last inserted row id.
     */
    auto execute(const std::string& query) -> int64_t;
    auto createStatement(const std::string& query) -> Statement;
    /**
     * @brief Queries the database to inspect whether the table with the
     * provided name exists.
     * @param tableName Name of the table.
     * @return True if the table exists, false otherwise.
     */
    [[nodiscard]] auto hasTable(const std::string& table) const -> bool;
};
} // namespace db

#endif // RHYTHMGAME_SQLITECPPDB_H
