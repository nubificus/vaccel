// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "include/ops/exec.h"
#include <stddef.h>

struct vaccel_session;
struct vaccel_arg;

int vaccel_exec_unpack(struct vaccel_session *sess, struct vaccel_arg *read,
		       int nr_read, struct vaccel_arg *write, int nr_write);

int vaccel_exec_with_res_unpack(struct vaccel_session *sess,
				struct vaccel_arg *read, int nr_read,
				struct vaccel_arg *write, int nr_write);
