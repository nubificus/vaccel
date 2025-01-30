// SPDX-License-Identifier: Apache-2.0

/*
 * The code below performs unit testing to log.
 *
 * 1) vaccel_log_init()
 * 2) vaccel_log_shutdown()
 *
 * Run with env var VACCEL_LOG_LEVEL=(1..4)
 *
 */

#include "vaccel.h"
#include <catch.hpp>
#include <cstdlib>
#include <cstring>

TEST_CASE("log_level", "[core][log]")
{
	int ret = vaccel_log_init();
	REQUIRE(ret == VACCEL_OK);
	ret = vaccel_log_shutdown();
	REQUIRE(ret == VACCEL_OK);
}
