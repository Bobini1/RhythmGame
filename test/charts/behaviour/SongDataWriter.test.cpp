#include <catch2/catch_test_macros.hpp>
#include "lua/Lua.h"
#include "charts/behaviour/SongDataWriterToLua.h"

TEST_CASE("Check if variables are correctly written to lua state",
          "[SongDataWriterToLua]")
{
    // note for using unicode - string literals MUST be used, otherwise sol will
    // throw an exception
    using namespace std::literals::string_literals;
    sol::state lua;
    charts::behaviour::SongDataWriterToLua sdw(lua);
    sdw.writeVar("testBpm", 180);
    REQUIRE(lua["testBpm"] == 180);
    sdw.writeVar("sleepy", true);
    REQUIRE(lua["sleepy"] == true);
    sdw.writeVar("unicodeTest", "日本語"s);
    REQUIRE(lua["unicodeTest"] == "日本語"s);
}