//
// Created by bobini on 11.07.2022.
//

#ifndef RHYTHMGAME_SQLITECPPDB_H
#define RHYTHMGAME_SQLITECPPDB_H
#include <SQLiteCpp/SQLiteCpp.h>
#include <optional>
#include <vector>
#include <type_traits>
#include <mutex>
#include "support/get.h"
#include "support/TupleSize.h"

/**
 * @brief Namespace for database related classes and functions.
 */
namespace db {

/**
 * @brief Database wrapper for SQLiteCpp.
 * @note This class is thread safe.
 */
class SqliteCppDb
{
    SQLite::Database db;

  public:
    /**
     * @brief Wrapper for SQLiteCpp::Statement.
     * @note This class is thread safe.
     */
    class Statement
    {
        SQLite::Statement statement;
        SQLite::Database* db;

      public:
        Statement(SQLite::Statement statement,
                  SQLite::Database* db);
        template<typename... T>
        auto bind(int index, T&&... values) -> void
        {
            statement.bind(index, std::forward<T>(values)...);
        }
        template<typename... T>
        auto bind(const std::string& name, T&&... values) -> void
        {
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
        {
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
        {
            std::vector<Ret> result;

            while (statement.executeStep()) {
                result.emplace_back();
                writeRow(statement, result.back());
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
     */
    explicit SqliteCppDb(const std::filesystem::path& dbPath);
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
     * @param table Name of the table.
     * @return True if the table exists, false otherwise.
     */
    [[nodiscard]] auto hasTable(const std::string& table) const -> bool;
};
} // namespace db

#endif // RHYTHMGAME_SQLITECPPDB_H
