// SPDX-License-Identifier: Apache-2.0

/*
 * Unit Testing for VAccel Noop Operation
 *
 * The code below performs unit testing for the VAccel Noop operation functions.
 *
 */

#include "vaccel.h"
#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <cstdio>
#include <cstdlib>

TEST_CASE("noop", "[ops][noop]")
{
	struct vaccel_session sess;
	REQUIRE(vaccel_session_init(&sess, 0) == VACCEL_OK);

	int const ret = vaccel_noop(&sess);
	REQUIRE(ret == VACCEL_OK);

	REQUIRE(vaccel_session_release(&sess) == VACCEL_OK);
}

TEST_CASE("noop_generic", "[ops][noop]")
{
	struct vaccel_session sess;
	REQUIRE(vaccel_session_init(&sess, 0) == VACCEL_OK);

	struct vaccel_arg_array read_args;
	REQUIRE(vaccel_arg_array_init(&read_args, 1) == VACCEL_OK);

	const auto op_type = (uint8_t)VACCEL_OP_NOOP;
	REQUIRE(vaccel_arg_array_add_uint8(&read_args, (uint8_t *)&op_type) ==
		VACCEL_OK);

	int const ret = vaccel_genop(&sess, read_args.args, read_args.count,
				     nullptr, 0);
	REQUIRE(ret == VACCEL_OK);

	REQUIRE(vaccel_session_release(&sess) == VACCEL_OK);
	REQUIRE(vaccel_arg_array_release(&read_args) == VACCEL_OK);
}
