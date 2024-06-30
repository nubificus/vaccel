// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

struct vaccel_session;
struct vaccel_arg;
struct vaccel_shared_object;

int vaccel_exec(struct vaccel_session *sess, const char *library,
		const char *fn_symbol, struct vaccel_arg *read, size_t nr_read,
		struct vaccel_arg *write, size_t nr_write);

int vaccel_exec_with_resource(struct vaccel_session *sess,
			      struct vaccel_shared_object *object,
			      const char *fn_symbol, struct vaccel_arg *read,
			      size_t nr_read, struct vaccel_arg *write,
			      size_t nr_write);

#ifdef __cplusplus
}
#endif
