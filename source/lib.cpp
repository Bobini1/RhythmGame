#include "lib.hpp"
#include <sol/sol.hpp>
#include <cassert>

library::library()
{
  sol::state lua;
  int x = 0;
  lua.set_function("beep", [&x]{ ++x; });
  lua.script("beep()");
  assert(x == 1);
}
