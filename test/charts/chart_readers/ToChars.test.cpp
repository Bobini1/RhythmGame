#include "charts/chart_readers/ToChars.h"
#include <catch2/catch_test_macros.hpp>

TEST_CASE("Check if getCh utility function returns the right char", "[single-file]")
{
    REQUIRE(getCh("testString", 0) == 't');
    REQUIRE(getCh("aBCdEf", 5) == 'f');
    REQUIRE(getCh("abc", 5) == '\0');
    REQUIRE(std::is_same_v<std::integer_sequence<char, RHYTHMGAME_TO_CHARS(10, "testString")>, std::integer_sequence<char, 't', 'e', 's', 't', 'S', 't', 'r', 'i', 'n', 'g'>>);
}

