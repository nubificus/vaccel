// SPDX-License-Identifier: Apache-2.0

/*
 * The code below performs unit testing to misc.
 *
 * 1) vaccel_get_plugins()
 *
 */

#include "vaccel.h"
#include <catch.hpp>
#include <fff.h>

DEFINE_FFF_GLOBALS;

TEST_CASE("get_plugins", "[core][misc]")
{
	struct vaccel_session session;
	session.id = 0;

	SECTION("return correct implementation")
	{
		int result = vaccel_get_plugins(&session, VACCEL_NO_OP);
		REQUIRE(result == VACCEL_OK);

		result = vaccel_get_plugins(&session, VACCEL_EXEC);
		REQUIRE(result == VACCEL_OK);

		result = vaccel_get_plugins(&session, VACCEL_BLAS_SGEMM);
		REQUIRE(result == VACCEL_OK);
	}
}
