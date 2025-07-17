// SPDX-License-Identifier: Apache-2.0

#include "noop.h"
#include "arg.h"
#include "error.h"
#include "log.h"
#include "op.h"
#include "plugin.h"
#include "prof.h"
#include "session.h"
#include <inttypes.h>
#include <stdint.h>

static struct vaccel_prof_region noop_op_stats =
	VACCEL_PROF_REGION_INIT("vaccel_noop_op");

typedef int (*noop_fn_t)(struct vaccel_session *sess);

int vaccel_noop(struct vaccel_session *sess)
{
	int ret;

	if (!sess)
		return VACCEL_EINVAL;

	vaccel_op_type_t op_type = VACCEL_OP_NOOP;
	op_debug_plugin_lookup(sess, op_type);

	vaccel_prof_region_start(&noop_op_stats);

	noop_fn_t plugin_noop = plugin_get_op_func(op_type, sess->hint);
	if (!plugin_noop)
		return VACCEL_ENOTSUP;

	ret = plugin_noop(sess);

	vaccel_prof_region_stop(&noop_op_stats);

	return ret;
}

int vaccel_noop_unpack(struct vaccel_session *sess, struct vaccel_arg *read,
		       int nr_read, struct vaccel_arg *write, int nr_write)
{
	if (nr_read || read) {
		vaccel_error("Wrong number of read arguments in noop: %d",
			     nr_read);
		return VACCEL_EINVAL;
	}

	if (nr_write || write) {
		vaccel_error("Wrong number of write arguments in noop: %d",
			     nr_write);
		return VACCEL_EINVAL;
	}

	return vaccel_noop(sess);
}

__attribute__((constructor)) static void vaccel_ops_init(void)
{
}

__attribute__((destructor)) static void vaccel_ops_fini(void)
{
	vaccel_prof_region_print(&noop_op_stats);
	vaccel_prof_region_release(&noop_op_stats);
}
