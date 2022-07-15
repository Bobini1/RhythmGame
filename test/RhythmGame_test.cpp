
#include <catch2/catch_test_macros.hpp>

static auto
factorial(int number) -> int
{
    return number <= 1 ? 1 : factorial(number - 1) * number; // fail
    // return number <= 1 ? 1      : Factorial( number - 1 ) * number;  // pass
}

TEST_CASE("Factorial of 0 is 1 (fail)", "[single-file]")
{
    REQUIRE(factorial(0) == 1);
}
