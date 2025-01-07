// SPDX-License-Identifier: Apache-2.0

/*
 * The code below performs unit testing for vaccel.c
 *
 * This tests the constructor and destructor aspect of the vaccel.c file.
 * However the destructor/shutdown functions has issues with testing at the 
 * current amount/
 *
 */

#include "vaccel.h"
#include <catch.hpp>
#include <cstdio>
#include <cstring>
#include <fff.h>

DEFINE_FFF_GLOBALS;

TEST_CASE("constructor_and_destructor", "[core][vaccel]")
{
	int ret;

	ret = plugins_cleanup();
	REQUIRE(ret == VACCEL_OK);

	ret = resources_cleanup();
	REQUIRE(ret == VACCEL_OK);

	ret = sessions_cleanup();
	REQUIRE(ret == VACCEL_OK);

	ret = vaccel_log_init();
	REQUIRE(ret == VACCEL_OK);

	ret = sessions_bootstrap();
	REQUIRE(ret == VACCEL_OK);

	ret = resources_bootstrap();
	REQUIRE(ret == VACCEL_OK);

	ret = plugins_bootstrap();
	REQUIRE(ret == VACCEL_OK);
}
