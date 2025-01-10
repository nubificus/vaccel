// SPDX-License-Identifier: Apache-2.0

#include "arg.h"
#include "error.h"
#include "log.h"
#include "op.h"
#include "plugin.h"
#include "session.h"
#include <inttypes.h>
#include <stdint.h>

typedef int (*opencv_fn_t)(struct vaccel_session *sess, struct vaccel_arg *read,
			   int nr_read, struct vaccel_arg *write, int nr_write);

int vaccel_opencv(struct vaccel_session *sess, struct vaccel_arg *read,
		  int nr_read, struct vaccel_arg *write, int nr_write)
{
	if (!sess)
		return VACCEL_EINVAL;

	vaccel_debug(
		"session:%" PRId64
		" Looking for plugin implementing the Optical Flow operation",
		sess->id);

	opencv_fn_t plugin_opencv =
		plugin_get_op_func(VACCEL_OP_OPENCV, sess->hint);
	if (!plugin_opencv)
		return VACCEL_ENOTSUP;

	return plugin_opencv(sess, read, nr_read, write, nr_write);
}

int vaccel_opencv_unpack(struct vaccel_session *sess, struct vaccel_arg *read,
			 int nr_read, struct vaccel_arg *write, int nr_write)
{
	//TODO: sanity check args
	/*
	if (nr_read != 2) {
		vaccel_error("Wrong number of read arguments in FOO: %d",
				nr_read);
		return VACCEL_EINVAL;
	}

	if (nr_write != 1) {
		vaccel_error("Wrong number of write arguments in FOO: %d",
				nr_write);
		return VACCEL_EINVAL;
	}
*/
	return vaccel_opencv(sess, read, nr_read, write, nr_write);
}
