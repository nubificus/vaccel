// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "vaccel/session.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

int vaccel_fpga_arraycopy(struct vaccel_session *sess, int a[], int out_a[],
			  size_t len_a);

int vaccel_fpga_mmult(struct vaccel_session *sess, float a[], float b[],
		      float c[], size_t len_a);

int vaccel_fpga_parallel(struct vaccel_session *sess, float a[], float b[],
			 float add_output[], float mult_output[], size_t len_a);

int vaccel_fpga_vadd(struct vaccel_session *sess, float a[], float b[],
		     float c[], size_t len_a, size_t len_b);

#ifdef __cplusplus
}
#endif
