// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "vaccel/session.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

int vaccel_fpga_arraycopy(struct vaccel_session *sess, int array[],
			  int out_array[], size_t len_array);

int vaccel_fpga_mmult(struct vaccel_session *sess, float A[], float B[],
		      float C[], size_t lenA);

int vaccel_fpga_parallel(struct vaccel_session *sess, float A[], float B[],
			 float add_output[], float mult_output[], size_t len_a);

int vaccel_fpga_vadd(struct vaccel_session *sess, float A[], float B[],
		     float C[], size_t len_a, size_t len_b);

#ifdef __cplusplus
}
#endif
