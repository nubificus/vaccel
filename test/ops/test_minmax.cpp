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

#include "utils.hpp"
#include "vaccel.h"
#include <bits/time.h>
#include <catch.hpp>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <ctime>

#define timespec_usec(t) \
	((double)(t).tv_nsec / 10e3 + (double)(t).tv_sec * 10e6)
#define time_diff_usec(t0, t1) (timespec_usec((t1)) - timespec_usec((t0)))

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
	session.id = 1;
	session.resources = nullptr;
	session.priv = nullptr;

	ret = vaccel_session_init(&session, 0);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(session.id);
	REQUIRE(session.hint == 0);
	REQUIRE(session.resources);
	REQUIRE(session.priv == nullptr);

	struct timespec t0;
	struct timespec t1;
	clock_gettime(CLOCK_MONOTONIC_RAW, &t0);

	ret = vaccel_minmax(&session, indata, ndata, low_threshold,
			    high_threshold, outdata, &min, &max);

	clock_gettime(CLOCK_MONOTONIC_RAW, &t1);
	REQUIRE(ret == VACCEL_OK);

	fprintf(stdout, "min: %lf max: %lf Execution time: %lf msec\n", min,
		max,
		((double)(t1.tv_sec - t0.tv_sec) * 1000.0) +
			((double)(t1.tv_nsec - t0.tv_nsec) / 1.0e6));

	ret = vaccel_session_release(&session);
	REQUIRE(session.id);
	REQUIRE(session.hint == 0);
	REQUIRE(session.resources == nullptr);
	REQUIRE(session.priv == nullptr);
	REQUIRE(ret == VACCEL_OK);

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
	session.id = 1;
	session.resources = nullptr;
	session.priv = nullptr;

	ret = vaccel_session_init(&session, 0);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(session.id);
	REQUIRE(session.hint == 0);
	REQUIRE(session.resources);
	REQUIRE(session.priv == nullptr);

	struct timespec t0;
	struct timespec t1;

	vaccel_op_t op_type = VACCEL_MINMAX;
	struct vaccel_arg read[5] = {
		{ .argtype = 0, .size = sizeof(vaccel_op_t), .buf = &op_type },
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
	clock_gettime(CLOCK_MONOTONIC_RAW, &t0);

	ret = vaccel_genop(&session, &read[0], 5, &write[0], 3);

	clock_gettime(CLOCK_MONOTONIC_RAW, &t1);
	REQUIRE(ret == VACCEL_OK);

	fprintf(stdout, "min: %lf max: %lf Execution time: %lf msec\n", min,
		max,
		((double)(t1.tv_sec - t0.tv_sec) * 1000.0) +
			((double)(t1.tv_nsec - t0.tv_nsec) / 1.0e6));

	ret = vaccel_session_release(&session);
	REQUIRE(session.id);
	REQUIRE(session.hint == 0);
	REQUIRE(session.resources == nullptr);
	REQUIRE(session.priv == nullptr);
	REQUIRE(ret == VACCEL_OK);

	ret = fclose(fp);
	REQUIRE(!ret);

	free(outdata);
	free(indata);
	free(path);
}
