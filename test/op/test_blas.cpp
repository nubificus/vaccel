/*
 * Unit Testing for BLAS Function (sgemm)
 *
 * The code below performs unit testing for the BLAS function sgemm.
 * This function computes the matrix-matrix product of a general matrix (C)
 * and the product of two matrices (A and B).
 *
 */

#include <catch.hpp>

#include <atomic>

using atomic_int = std::atomic<int>;
using atomic_uint = std::atomic<unsigned int>;

extern "C" {
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <vaccel.h>

#include "blas.h"
#include "session.h"
}

extern "C" {
#define M 512
#define N 512
#define K 512
#define ELEM_2D(array, i, j, ld) (*((array) + i * ld + j))

void init(int m, int n, int k, float* A, float* B, float* C)
{
    for (int i = 0; i < m; ++i)
        for (int j = 0; j < k; ++j)
            ELEM_2D(A, i, j, k) = ((float)i * j) / m;

    for (int i = 0; i < k; ++i)
        for (int j = 0; j < n; ++j)
            ELEM_2D(B, i, j, n) = ((float)i * j) / m;

    for (int i = 0; i < m; ++i)
        for (int j = 0; j < n; ++j)
            ELEM_2D(C, i, j, n) = ((float)i * j) / m;
}
}

TEST_CASE("sgemm", "[blas]")
{

    // Open a file stream for output data
    // FILE* data_fp = NULL;

    // const char* path_output = "../../test/empty.txt";
    // data_fp = fopen(path_output, "wb");
    // REQUIRE(data_fp);

    // Initialize matrices A, B, and C with specific values
    float alpha = 32412.0, beta = 2123.0;
    float A[M * K];
    float B[K * N];
    float C[M * N];
    init(M, N, K, A, B, C);

    // Initialize a VAccel session
    struct vaccel_session session;
    session.session_id = 1;
    session.resources = nullptr;
    session.priv = nullptr;

    int ret = vaccel_sess_init(&session, 0);
    REQUIRE(ret == VACCEL_OK);
    REQUIRE(session.session_id);
    REQUIRE(session.hint == 0);
    REQUIRE(session.resources);
    REQUIRE(session.priv == nullptr);
    // Invoke the sgemm function with the initialized matrices and session
    ret = vaccel_sgemm(&session, M, N, K, alpha, (float*)A, K, (float*)B, N,
        beta, (float*)C, N);
    REQUIRE(ret == VACCEL_OK);

    // Write the result matrix C to a file
    // fwrite(C, sizeof(float), M * N, data_fp);

    // Free the VAccel session
    ret = vaccel_sess_free(&session);

    REQUIRE(session.session_id);
    REQUIRE(session.hint == 0);
    REQUIRE(session.resources == nullptr);
    REQUIRE(session.priv == nullptr);
    REQUIRE(ret == VACCEL_OK);
}
