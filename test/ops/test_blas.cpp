// SPDX-License-Identifier: Apache-2.0

/*
 * Unit Testing for BLAS Function (sgemm)
 *
 * The code below performs unit testing for the BLAS function sgemm.
 * This function computes the matrix-matrix product of a general matrix (C)
 * and the product of two matrices (A and B).
 *
 */

#include "vaccel.h"
#include <catch.hpp>
#include <cerrno>
#include <cstdio>
#include <cstdlib>

extern "C" {

#define ELEM_2D(array, i, j, ld) (*((array) + (ptrdiff_t)((i) * (ld)) + (j)))

enum { M = 512, N = 512, K = 512 };

void init(long long int m, long long int n, long long int k, float *A, float *B,
	  float *C)
{
	for (long long int i = 0; i < m; ++i)
		for (long long int j = 0; j < k; ++j)
			ELEM_2D(A, i, j, k) = ((float)i * (float)j) / (float)m;

	for (long long int i = 0; i < k; ++i)
		for (long long int j = 0; j < n; ++j)
			ELEM_2D(B, i, j, n) = ((float)i * (float)j) / (float)m;

	for (long long int i = 0; i < m; ++i)
		for (long long int j = 0; j < n; ++j)
			ELEM_2D(C, i, j, n) = ((float)i * (float)j) / (float)m;
}
}

TEST_CASE("sgemm", "[ops][blas]")
{
	float const alpha = 32412.0;
	float const beta = 2123.0;
	float A[M * K];
	float B[K * N];
	float C[M * N];
	init(M, N, K, A, B, C);

	struct vaccel_session session;
	REQUIRE(vaccel_session_init(&session, 0) == VACCEL_OK);

	int ret = vaccel_sgemm(&session, M, N, K, alpha, (float *)A, K,
			       (float *)B, N, beta, (float *)C, N);
	REQUIRE(ret == VACCEL_OK);

	REQUIRE(vaccel_session_release(&session) == VACCEL_OK);
}
