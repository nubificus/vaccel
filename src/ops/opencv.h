// SPDX-License-Identifier: Apache-2.0

#pragma once

//#include "include/vaccel/ops/optflow.h"
#include "session.h"
#include "arg.h"
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

//int vaccel_optflow(struct vaccel_session *sess, struct vaccel_arg *read,
//int nr_read, struct vaccel_arg *write, int nr_write);

int vaccel_opencv_unpack(struct vaccel_session *sess, struct vaccel_arg *read,
			 int nr_read, struct vaccel_arg *write, int nr_write);

#ifdef __cplusplus
}
#endif
