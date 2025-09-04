// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "vaccel/session.h"
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Matrix-Matrix multiplication for single-precision */
int vaccel_sgemm(struct vaccel_session *sess, int64_t m, int64_t n, int64_t k,
		 float alpha, float *a, int64_t lda, float *b, int64_t ldb,
		 float beta, float *c, int64_t ldc);

#ifdef __cplusplus
}
#endif
