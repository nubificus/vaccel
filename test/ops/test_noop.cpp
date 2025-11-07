// SPDX-License-Identifier: Apache-2.0

/*
 * Unit Testing for VAccel Noop Operation
 *
 * The code below performs unit testing for the VAccel Noop operation functions.
 *
 */

#include "vaccel.h"
#include <catch2/catch_test_macros.hpp>
#include <cstdio>
#include <cstdlib>

TEST_CASE("noop", "[ops][noop]")
{
	int ret;
	struct vaccel_session sess;

	ret = vaccel_session_init(&sess, 0);
	REQUIRE(ret == VACCEL_OK);

	ret = vaccel_noop(&sess);
	REQUIRE(ret == VACCEL_OK);

	ret = vaccel_session_release(&sess);
	REQUIRE(ret == VACCEL_OK);
}
