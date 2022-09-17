//
// Created by bobini on 11.07.2022.
//

#ifndef RHYTHMGAME_SQLITECPPDB_H
#define RHYTHMGAME_SQLITECPPDB_H
#include <SQLiteCpp/SQLiteCpp.h>
#include <optional>
#include <vector>
#include <map>
#include <db/sqlite_cpp_db/DatabaseAccessPoint.h>

namespace db::sqlite_cpp_db {

class SqliteCppDb
{
    thread_local static DatabaseAccessPoint connections;
    std::string connKey;

  public:
    explicit SqliteCppDb(std::string dbPath);
    [[nodiscard]] auto hasTable(const std::string& table) const -> bool;
    auto execute(const std::string& query) const -> void;

    template<typename... Ret>
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
} // namespace db::sqlite_cpp_db

#endif // RHYTHMGAME_SQLITECPPDB_H
