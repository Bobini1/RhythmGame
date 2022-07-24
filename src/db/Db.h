//
// Created by PC on 05/07/2022.
//

#ifndef RHYTHMGAME_DB_H
#define RHYTHMGAME_DB_H

#include <string>
#include <any>
#include <optional>
#include <vector>

namespace db {
class Db
{
  public:
    virtual ~Db() = default;
    Db() = default;
    Db(const Db&) = default;
    auto operator=(const Db&) -> Db& = default;
    Db(Db&&) = default;
    auto operator=(Db&&) -> Db& = default;
    
    [[nodiscard]] virtual auto hasTable(const std::string& table) const
      -> bool = 0;
    virtual auto execute(const std::string& query) const -> void = 0;
    [[nodiscard]] virtual auto executeAndGet(const std::string& query) const
      -> std::optional<std::any> = 0;
    [[nodiscard]] virtual auto executeAndGetAll(const std::string& query) const
      -> std::vector<std::any> = 0;
};

} // namespace db

#endif // RHYTHMGAME_DB_H
