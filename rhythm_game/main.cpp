#include <iostream>
#include <memory>

#include <DB.h>

auto main() -> int
{
  auto db = rhythm_game::db::create_db();
  db->insert("key", "value");
  std::cout << db->get("key") << std::endl;
}
