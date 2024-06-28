
#include <catch.hpp>
#include <utils.hpp>

extern "C" {
#include "error.h"
}

TEST_CASE("template", "[temp]")
{
	int ret = 0;
	REQUIRE(ret == VACCEL_OK);
}