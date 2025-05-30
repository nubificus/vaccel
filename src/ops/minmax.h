// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "include/vaccel/ops/minmax.h" // IWYU pragma: export
#include "arg.h"
#include "session.h"

#ifdef __cplusplus
extern "C" {
#endif

int vaccel_minmax_unpack(struct vaccel_session *sess, struct vaccel_arg *read,
			 int nr_read, struct vaccel_arg *write, int nr_write);

#ifdef __cplusplus
}
#endif
