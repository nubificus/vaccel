#include <catch2/catch_test_macros.hpp>

#include <atomic>

using atomic_int = std::atomic<int>;
using atomic_uint = std::atomic<unsigned int>;

extern "C"
{
#include <vaccel.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <errno.h>

#include "session.h"
#include "blas.h"
}

extern "C"
{
#define M 512
#define N 512
#define K 512
#define ELEM_2D(array, i, j, ld) (*((array) + i*ld + j))

void init(int m, int n, int k, float *A, float *B, float *C)
{
	for (int i = 0; i < m; ++i)
		for (int j = 0; j < k; ++j)
			ELEM_2D(A, i, j, k) = ((float) i * j) / m;

	for (int i = 0; i < k; ++i)
		for (int j = 0; j < n; ++j)
			ELEM_2D(B, i, j, n) = ((float) i * j) / m;

	for (int i = 0; i < m; ++i)
		for (int j = 0; j < n; ++j)
			ELEM_2D(C, i, j, n) = ((float) i * j) / m;
}
}

TEST_CASE("sgemm", "[blas]"){

    FILE *data_fp = NULL;
    const char *path_output = "../../test/input/output.txt";

    data_fp = fopen(path_output, "wb");
    REQUIRE(data_fp);

    float alpha = 32412.0, beta = 2123.0;
	float A[M*K];
	float B[K*N];
	float C[M*N];
	init(M, N, K, A, B, C);

    struct vaccel_session session;
    int ret = vaccel_sess_init(&session, 0);
    REQUIRE(ret == VACCEL_OK);

    ret = vaccel_sgemm(&session,M, N, K,alpha,(float *)A, K,(float *)B, N,beta,(float *)C, N);
    REQUIRE(ret == VACCEL_OK);

    //fwrite(C, sizeof(float), M * N, data_fp);
    ret = vaccel_sess_free(&session);
    REQUIRE(ret == VACCEL_OK);

}