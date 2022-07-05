
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MAIN  // in only one cpp file

#include <catch2/catch_test_macros.hpp>

static int Factorial( int number ) {
  return number <= 1 ? 1 : Factorial( number - 1 ) * number;  // fail
  // return number <= 1 ? 1      : Factorial( number - 1 ) * number;  // pass
}

TEST_CASE( "Factorial of 0 is 1 (fail)", "[single-file]" ) {
  REQUIRE( Factorial(0) == 1 );
}