// SPDX-License-Identifier: Apache-2.0

/*
 * The code below performs unit testing for vaccel.c
 *
 * This tests the constructor and destructor aspect of the vaccel.c file.
 * However the destructor/shutdown functions has issues with testing at the 
 * current amount/
 */

#include <catch.hpp>
#include <fff.h>
#include <utils.hpp>

DEFINE_FFF_GLOBALS;

#include <dlfcn.h>

#include <cstdio>
#include <cstring>
#include <sys/stat.h>
#include <unistd.h>
#include <vaccel.h>

TEST_CASE("constructor_and_destructor", "[core][vaccel]")
{
	int ret;

	ret = plugins_shutdown();
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
