//
// Created by PC on 05/07/2022.
//

#ifndef RHYTHMGAME_DB_H
#define RHYTHMGAME_DB_H

#include <memory>
#include <string>

namespace rhythm_game::db
{
class db
{
public:
  virtual ~db() = default;
  virtual auto insert(const std::string& key, const std::string& value)
      -> void = 0;
  virtual auto get(const std::string& key) -> std::string = 0;
};

auto create_db() -> std::unique_ptr<db>;
}  // namespace rhythm_game::db

#endif  // RHYTHMGAME_DB_H
