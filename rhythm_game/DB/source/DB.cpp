//
// Created by PC on 05/07/2022.
//

#include <iostream>

#include "DB.h"
namespace rhythm_game::db
{

class redis_db : public db
{
public:
  virtual ~redis_db() override = default;
  virtual auto insert(const std::string& key, const std::string& value)
      -> void override
  {
    std::cout << "redis_db::insert()" << std::endl;
  }
  virtual auto get(const std::string& key) -> std::string override
  {
    std::cout << "redis_db::get()" << std::endl;
    return "";
  }
};

auto create_db() -> std::unique_ptr<db>
{
  return std::make_unique<redis_db>();
}
}  // namespace rhythm_game::db
