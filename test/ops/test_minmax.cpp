// SPDX-License-Identifier: Apache-2.0

/*
 * Unit Testing for VAccel Min-Max Operation
 *
 * The code below performs unit testing for the VAccel Min-Max operation.
 * It includes two test cases: one for the standard Min-Max operation and
 * another for the generic version using vaccel_genop. Both test cases read
 * input data from a CSV file, perform the Min-Max operation, and print the
 * results along with the execution time.
 *
 */

#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

#include "utils.hpp"
#include "vaccel.h"
#include <catch.hpp>
#include <cstdint>
#include <cstdio>
#include <cstdlib>

TEST_CASE("min_max", "[ops][minmax]")
{
	char *path = abs_path(SOURCE_ROOT, "examples/input/input_2048.csv");
	double min;
	double max;
	int ret;

	int const ndata = 2048;
	REQUIRE(ndata > 0);

	auto *indata = (double *)malloc(ndata * sizeof(double));
	REQUIRE(indata != nullptr);

	FILE *fp = fopen(path, "r");
	REQUIRE(fp != nullptr);

	for (int i = 0; i < ndata; ++i) {
		REQUIRE(fscanf(fp, "%lf\n", &indata[i]) == 1);
	}

	auto *outdata = (double *)malloc(ndata * sizeof(double));
	REQUIRE(outdata != nullptr);

	int const low_threshold = 0;
	int const high_threshold = 4000;

	struct vaccel_session session;
	REQUIRE(vaccel_session_init(&session, 0) == VACCEL_OK);

	ret = vaccel_minmax(&session, indata, ndata, low_threshold,
			    high_threshold, outdata, &min, &max);
	REQUIRE(ret == VACCEL_OK);

	REQUIRE(vaccel_session_release(&session) == VACCEL_OK);

	ret = fclose(fp);
	REQUIRE(!ret);

	free(outdata);
	free(indata);
	free(path);
}

TEST_CASE("min_max_generic", "[ops][minmax]")
{
	char *path = abs_path(SOURCE_ROOT, "examples/input/input_2048.csv");
	double min;
	double max;
	int ret;

	int ndata = 2048;
	REQUIRE(ndata > 0);

	auto *indata = (double *)malloc(ndata * sizeof(double));
	REQUIRE(indata != nullptr);

	FILE *fp = fopen(path, "r");
	REQUIRE(fp != nullptr);

	for (int i = 0; i < ndata; ++i) {
		REQUIRE(fscanf(fp, "%lf\n", &indata[i]) == 1);
	}

	auto *outdata = (double *)malloc(ndata * sizeof(double));
	REQUIRE(outdata != nullptr);

	int low_threshold = 0;
	int high_threshold = 4000;

	struct vaccel_session session;
	REQUIRE(vaccel_session_init(&session, 0) == VACCEL_OK);

	vaccel_op_type_t op_type = VACCEL_OP_MINMAX;
	struct vaccel_arg read[5] = {
		{ .argtype = 0,
		  .size = sizeof(vaccel_op_type_t),
		  .buf = &op_type },
		{ .argtype = 0,
		  .size = static_cast<uint32_t>(ndata * sizeof(double)),
		  .buf = indata },
		{ .argtype = 0, .size = sizeof(int), .buf = &ndata },
		{ .argtype = 0, .size = sizeof(int), .buf = &low_threshold },
		{ .argtype = 0, .size = sizeof(int), .buf = &high_threshold },
	};
	struct vaccel_arg write[3] = {
		{ .argtype = 0,
		  .size = static_cast<uint32_t>(ndata * sizeof(double)),
		  .buf = outdata },
		{ .argtype = 0, .size = sizeof(double), .buf = &min },
		{ .argtype = 0, .size = sizeof(double), .buf = &max },
	};

	ret = vaccel_genop(&session, &read[0], 5, &write[0], 3);
	REQUIRE(ret == VACCEL_OK);

	REQUIRE(vaccel_session_release(&session) == VACCEL_OK);

	ret = fclose(fp);
	REQUIRE(!ret);

	free(outdata);
	free(indata);
	free(path);
}
