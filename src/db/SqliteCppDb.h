//
// Created by bobini on 11.07.2022.
//

#ifndef RHYTHMGAME_SQLITECPPDB_H
#define RHYTHMGAME_SQLITECPPDB_H
#include <SQLiteCpp/SQLiteCpp.h>
#include <optional>
#include <vector>
#include <map>
#include <db/DatabaseAccessPoint.h>

namespace db {

/**
 * @brief Database wrapper for SQLiteCpp.
 * The wrapper uses thread_local database connection objects to provide
 * lock-free access.
 */
class SqliteCppDb
{
    thread_local static DatabaseAccessPoint connections;
    std::string connKey;

  public:
    /**
     * @brief Constructs a database wrapper.
     * @param dbPath Path to the database file.
     * The database file will be created if it does not exist.
     */
    explicit SqliteCppDb(std::string dbPath);

    /**
     * @brief Queries the database to inspect whether the table with the
     * provided name exists.
     * @param tableName Name of the table.
     * @return True if the table exists, false otherwise.
     */
    [[nodiscard]] auto hasTable(const std::string& table) const -> bool;
    auto execute(const std::string& query) const -> void;

    /**
     * @brief Executes a query that returns a single row.
     * @param query Query to execute.
     * @return An optional holding the result of the query. It will be empty if
     * the query didn't return anything. If the query returns fewer columns than
     * specified in template parameters, they will be default initialized.
     * @tparam Ret Types of the columns. Must be default constructible.
     */
    template<std::default_initializable... Ret>
    [[nodiscard]] auto executeAndGet(const std::string& query) const
      -> std::optional<std::tuple<Ret...>>
    {
        SQLite::Statement statement(connections[connKey], query);
        if (!statement.executeStep()) {
            return {};
        }
        std::tuple<Ret...> result;
        constexpr size_t tupleSize = std::tuple_size_v<std::tuple<Ret...>>;
        constexpr auto indices =
          std::make_integer_sequence<int, static_cast<int>(tupleSize)>();

        auto lambda = [&statement]<typename Elem>(Elem& elem, int index) {
            // This if is necessary for MSVC (don't ask me why)
            if constexpr (std::is_same_v<Elem, std::string>) {
                elem = statement.getColumn(index).getString();
            } else {
                elem = static_cast<Elem>(statement.getColumn(index));
            }
        };
        auto outerLambda =
          [&lambda, &result ]<int... N>(std::integer_sequence<int, N...>)
        {
            (lambda(std::get<N>(result), N), ...);
        };

        outerLambda(indices);

        return result;
    }
    /**
     * @brief Executes a query that returns any number of rows.
     * @param query Query to execute.
     * @return A vector holding the result of the query. It will be empty if
     * the query didn't return anything. If the query returns fewer columns than
     * specified in the template parameters, this method will throw.
     * @tparam Ret Types of the columns. Must be default constructible.
     */
    template<typename... Ret>
    [[nodiscard]] auto executeAndGetAll(const std::string& query) const
      -> std::vector<std::tuple<Ret...>>
    {
        SQLite::Statement statement(connections[connKey], query);
        std::vector<std::tuple<Ret...>> result;
        constexpr size_t tupleSize = std::tuple_size_v<std::tuple<Ret...>>;
        constexpr auto indices =
          std::make_integer_sequence<int, static_cast<int>(tupleSize)>();

        auto lambda = [&statement]<typename Elem>(Elem& elem, int index) {
            // This if is necessary for MSVC (don't ask me why)
            if constexpr (std::is_same_v<Elem, std::string>) {
                elem = statement.getColumn(index).getString();
            } else {
                elem = static_cast<Elem>(statement.getColumn(index));
            }
        };
        auto outerLambda =
          [&lambda, &result ]<int... N>(std::integer_sequence<int, N...>)
        {
            (lambda(std::get<N>(result.back()), N), ...);
        };
        while (statement.executeStep()) {
            result.emplace_back();
            outerLambda(indices);
        }

        return result;
    }
};
} // namespace db

#endif // RHYTHMGAME_SQLITECPPDB_H
