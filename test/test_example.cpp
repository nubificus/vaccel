/*
 * Unit Testing example
 *
 * Here is an example for a unit test
 *
 */

#include <catch.hpp>
#include <cstdint>

auto factorial(uint32_t number) -> uint32_t
{
    return number <= 1 ? number : factorial(number - 1) * number;
}

TEST_CASE("Factorials are computed", "[factorial]")
{
    REQUIRE(factorial(1) == 1);
    REQUIRE(factorial(2) == 2);
    REQUIRE(factorial(3) == 6);
    REQUIRE(factorial(4) == 24);
    REQUIRE(factorial(10) == 3628800);
}
