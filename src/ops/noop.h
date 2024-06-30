// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "include/ops/noop.h"

struct vaccel_session;
struct vaccel_arg;

int vaccel_noop_unpack(struct vaccel_session *sess, struct vaccel_arg *read,
		       int nr_read, struct vaccel_arg *write, int nr_write);
