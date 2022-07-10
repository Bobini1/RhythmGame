#include <iostream>
#include <memory>

#include "db/DB.h"

auto main() -> int
{
  auto db = db::create_db();
  db->insert("key", "value");
  std::cout << db->get("key") << std::endl;
}
