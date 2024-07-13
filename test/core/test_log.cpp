// SPDX-License-Identifier: Apache-2.0

/*
 * The code below performs unit testing to log.
 *
 * 1) vaccel_log_init()
 * 2) vaccel_log_shutdown()
 *
 * Run with env var VACCEL_DEBUG_LEVEL=(1..4)
 */

#include <catch.hpp>
#include <utils.hpp>

#include <cstdlib>
#include <cstring>
#include <vaccel.h>

TEST_CASE("log_level", "[log]")
{
	int ret = vaccel_log_init();
	REQUIRE(ret == VACCEL_OK);
	ret = vaccel_log_shutdown();
	REQUIRE(ret == VACCEL_OK);
}
