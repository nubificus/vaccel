// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct vaccel_session;

/* Matrix-Matrix multiplication for single-precision */
int vaccel_sgemm(struct vaccel_session *sess, long long int m, long long int n,
		 long long int k, float alpha, float *a, long long int lda,
		 float *b, long long int ldb, float beta, float *c,
		 long long int ldc);

#ifdef __cplusplus
}
#endif
