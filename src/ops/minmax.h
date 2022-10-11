#pragma once


#include "include/ops/minmax.h"

int vaccel_minmax_unpack(struct vaccel_session *sess, struct vaccel_arg *read,
		int nr_read, struct vaccel_arg *write, int nr_write);
