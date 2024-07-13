// SPDX-License-Identifier: Apache-2.0

/*
 * Unit Testing for VAccel Noop Operation
 *
 * The code below performs unit testing for the VAccel Noop operation functions.
 */

#include <catch.hpp>
#include <utils.hpp>

#include <cstdio>
#include <cstdlib>
#include <vaccel.h>

TEST_CASE("noop", "[ops_noop]")
{
	int ret;
	struct vaccel_session sess;

	ret = vaccel_sess_init(&sess, 0);
	REQUIRE(ret == VACCEL_OK);

	ret = vaccel_noop(&sess);
	REQUIRE(ret == VACCEL_OK);

	ret = vaccel_sess_free(&sess);
	REQUIRE(ret == VACCEL_OK);
}
