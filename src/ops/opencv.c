// SPDX-License-Identifier: Apache-2.0

#include "arg.h"
#include "error.h"
#include "op.h"
#include "plugin.h"
#include "prof.h"
#include "session.h"
#include <inttypes.h>
#include <stdint.h>

static struct vaccel_prof_region opencv_op_stats =
	VACCEL_PROF_REGION_INIT("vaccel_opencv_op");

typedef int (*opencv_fn_t)(struct vaccel_session *sess, struct vaccel_arg *read,
			   int nr_read, struct vaccel_arg *write, int nr_write);

int vaccel_opencv(struct vaccel_session *sess, struct vaccel_arg *read,
		  int nr_read, struct vaccel_arg *write, int nr_write)
{
	int ret;

	if (!sess)
		return VACCEL_EINVAL;

	vaccel_op_type_t op_type = VACCEL_OP_OPENCV;
	op_debug_plugin_lookup(sess, op_type);

	vaccel_prof_region_start(&opencv_op_stats);

	opencv_fn_t plugin_opencv = plugin_get_op_func(op_type, sess->hint);
	if (!plugin_opencv) {
		ret = VACCEL_ENOTSUP;
		goto out;
	}

	ret = plugin_opencv(sess, read, nr_read, write, nr_write);

out:
	vaccel_prof_region_stop(&opencv_op_stats);

	return ret;
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

__attribute__((constructor)) static void vaccel_ops_init(void)
{
}

__attribute__((destructor)) static void vaccel_ops_fini(void)
{
	vaccel_prof_region_print(&opencv_op_stats);
	vaccel_prof_region_release(&opencv_op_stats);
}
