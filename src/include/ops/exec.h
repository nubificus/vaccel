// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "resource.h"
#include "session.h"
#include "vaccel_args.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

int vaccel_exec(struct vaccel_session *sess, const char *library,
		const char *fn_symbol, struct vaccel_arg *read, size_t nr_read,
		struct vaccel_arg *write, size_t nr_write);

int vaccel_exec_with_resource(struct vaccel_session *sess,
			      struct vaccel_resource *resource,
			      const char *fn_symbol, struct vaccel_arg *read,
			      size_t nr_read, struct vaccel_arg *write,
			      size_t nr_write);

#ifdef __cplusplus
}
#endif
