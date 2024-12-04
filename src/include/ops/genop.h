// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "session.h"
#include "vaccel_args.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Call one of the supported functions, given an op code and a set of arbitrary
 * arguments */
int vaccel_genop(struct vaccel_session *sess, struct vaccel_arg *read,
		 int nr_read, struct vaccel_arg *write, int nr_write);

#ifdef __cplusplus
}
#endif
