#include "lib.hpp"
#include <sol/sol.hpp>
#include <cassert>
#include <fmt/core.h>
#include <folly/FBString.h>

library::library()
{
  sol::state lua;
  int x = 0;
  lua.set_function("beep", [&x]{ ++x; });
  lua.script("beep()");
  assert(x == 1);
}
