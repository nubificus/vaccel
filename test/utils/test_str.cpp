// SPDX-License-Identifier: Apache-2.0

/*
 * The code below performs unit testing to `str` functions.
 *
 * 1) vaccel_str_to_lower()
 *
 */

#include "vaccel.h"
#include <catch.hpp>
#include <cstdlib>
#include <cstring>

TEST_CASE("vaccel_str_to_lower", "[utils][str]")
{
	int ret;
	const char str[] = "HELLO WORLD";
	const char *str_lower = "hello world";
	char lower[16];
	char *alloc_lower;

	SECTION("success")
	{
		ret = vaccel_str_to_lower(str, lower, sizeof(lower), nullptr);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(strcmp(lower, str_lower) == 0);
	}

	SECTION("success with shorter output")
	{
		char lower_short[6];
		ret = vaccel_str_to_lower(str, lower_short, sizeof(lower_short),
					  nullptr);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(strcmp(lower_short, "hello") == 0);
	}

	SECTION("alloc success")
	{
		ret = vaccel_str_to_lower(str, nullptr, 0, &alloc_lower);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(strcmp(alloc_lower, str_lower) == 0);
		free(alloc_lower);
	}

	SECTION("invalid arguments")
	{
		ret = vaccel_str_to_lower(nullptr, lower, sizeof(lower),
					  &alloc_lower);
		REQUIRE(ret == VACCEL_EINVAL);

		ret = vaccel_str_to_lower(str, nullptr, sizeof(lower), nullptr);
		REQUIRE(ret == VACCEL_EINVAL);

		ret = vaccel_str_to_lower(str, lower, 0, nullptr);
		REQUIRE(ret == VACCEL_EINVAL);

		ret = vaccel_str_to_lower(str, nullptr, 0, nullptr);
		REQUIRE(ret == VACCEL_EINVAL);
	}
}
