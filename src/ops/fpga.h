// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "include/ops/fpga.h" // IWYU pragma: export
#include "session.h"
#include "vaccel_args.h"
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int vaccel_fpga_arraycopy_unpack(struct vaccel_session *sess,
				 struct vaccel_arg *read, int nr_read,
				 struct vaccel_arg *write, int nr_write);

int vaccel_fpga_mmult_unpack(struct vaccel_session *sess,
			     struct vaccel_arg *read, int nr_read,
			     struct vaccel_arg *write, int nr_write);

int vaccel_fpga_parallel_unpack(struct vaccel_session *sess,
				struct vaccel_arg *read, int nr_read,
				struct vaccel_arg *write, int nr_write);

int vaccel_fpga_vadd_unpack(struct vaccel_session *sess,
			    struct vaccel_arg *read, int nr_read,
			    struct vaccel_arg *write, int nr_write);

#ifdef __cplusplus
}
#endif
