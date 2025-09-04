// SPDX-License-Identifier: Apache-2.0

/*
 * Unit Testing for VAccel FPGA Plugins
 *
 * The code below performs unit testing for VAccel FPGA plugins.
 * It includes test cases for the `vaccel_fpga_add`, `vaccel_fpga_copy`,
 * `vaccel_fpga_mmult`, and `vaccel_fpga_parallel` functions.
 *
 */

#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

#include "vaccel.h"
#include <algorithm>
#include <catch.hpp>
#include <cstddef>
#include <cstdlib>
#include <iostream>

TEST_CASE("fpga_add", "[ops][fpga]")
{
	int ret;
	struct vaccel_session sess;

	float A[] = { 1, 2, 3, 4, 5 };
	float B[] = { 1, 2, 6, 4, 5 };
	float C[] = { 1, 2, 6, 4, 5 };

	size_t const len_A = sizeof(A) / sizeof(A[0]);
	size_t const len_B = sizeof(B) / sizeof(B[0]);

	const char *vaccel_plugins = std::getenv("VACCEL_PLUGINS");

	if (vaccel_plugins != nullptr) {
		std::cout << "VACCEL_PLUGINS: " << vaccel_plugins << '\n';
	} else {
		std::cerr << "VACCEL_PLUGINS environment variable not set."
			  << '\n';
	}

	SECTION("null session")
	{
		REQUIRE(vaccel_fpga_vadd(nullptr, A, B, C, len_A, len_B) ==
			VACCEL_EINVAL);
	}

	SECTION("valid session and inputs")
	{
		ret = vaccel_session_init(&sess, 0);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(sess.id);
		REQUIRE(sess.hint == 0);
		REQUIRE(sess.resources);
		REQUIRE(sess.priv == nullptr);

		ret = vaccel_fpga_vadd(&sess, A, B, C, len_A, len_B);
		float C_expected[] = { 2, 4, 9, 8, 10 };
		size_t const len_C = sizeof(C) / sizeof(C[0]);
		size_t const len_C_expected =
			sizeof(C_expected) / sizeof(C_expected[0]);

		REQUIRE(vaccel_session_release(&sess) == VACCEL_OK);
		REQUIRE(ret != VACCEL_ENOTSUP);

		REQUIRE(len_C == len_C_expected);

		REQUIRE(std::equal(C, C + len_C, C_expected));
	}
}

TEST_CASE("fpga_copy", "[ops][fpga]")
{
	int ret;
	struct vaccel_session sess;

	int A[] = { 1, 2, 3, 4, 5 };
	int B[] = { 1, 1, 1, 1, 1 };

	size_t const len_A = sizeof(A) / sizeof(A[0]);
	size_t const len_B = sizeof(B) / sizeof(B[0]);

	SECTION("null session")
	{
		REQUIRE(vaccel_fpga_arraycopy(nullptr, A, B, len_A) ==
			VACCEL_EINVAL);
	}

	SECTION("valid session and inputs")
	{
		ret = vaccel_session_init(&sess, 0);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(sess.id);
		REQUIRE(sess.hint == 0);
		REQUIRE(sess.resources);
		REQUIRE(sess.priv == nullptr);
		ret = vaccel_fpga_arraycopy(&sess, A, B, len_A);
		float B_expected[] = { 1, 2, 3, 4, 5 };

		REQUIRE(vaccel_session_release(&sess) == VACCEL_OK);
		REQUIRE(ret != VACCEL_ENOTSUP);

		REQUIRE(std::equal(B, B + len_B, B_expected));
	}
}

TEST_CASE("fpga_mmult", "[ops][fpga]")
{
	int ret;
	struct vaccel_session sess;

	float a[] = { 1.2, 3.2, 3.0, 4.1, 5.7 };
	float b[] = { 1.1, 0.2, 6.1, 4.6, 5.2 };

	float c[] = { 0.1, 0.1, 0.1, 0.1, 0.1 };
	size_t const len_c = sizeof(c) / sizeof(c[0]);

	SECTION("null session")
	{
		REQUIRE(vaccel_fpga_mmult(nullptr, a, b, c, len_c) ==
			VACCEL_EINVAL);
	}

	SECTION("valid session and inputs")
	{
		ret = vaccel_session_init(&sess, 0);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(sess.id);
		REQUIRE(sess.hint == 0);
		REQUIRE(sess.resources);
		REQUIRE(sess.priv == nullptr);

		ret = vaccel_fpga_mmult(&sess, a, b, c, len_c);
		float C_expected[] = { 9.1, 9.1, 9.1, 9.1, 9.1 };

		REQUIRE(vaccel_session_release(&sess) == VACCEL_OK);
		REQUIRE(ret != VACCEL_ENOTSUP);

		REQUIRE(std::equal(c, c + len_c, C_expected));
	}
}
