// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "include/ops/exec.h" // IWYU pragma: export
#include "session.h"
#include "vaccel_args.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

int vaccel_exec_unpack(struct vaccel_session *sess, struct vaccel_arg *read,
		       int nr_read, struct vaccel_arg *write, int nr_write);

int vaccel_exec_with_res_unpack(struct vaccel_session *sess,
				struct vaccel_arg *read, int nr_read,
				struct vaccel_arg *write, int nr_write);

#ifdef __cplusplus
}
#endif
