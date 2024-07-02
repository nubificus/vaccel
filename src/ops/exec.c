// SPDX-License-Identifier: Apache-2.0

#include <stdio.h>

#include "exec.h"
#include "error.h"
#include "genop.h"
#include "log.h"
#include "plugin.h"
#include "resources/shared_object.h"
#include "session.h"
#include "vaccel_ops.h"
#include "vaccel_prof.h"

struct vaccel_prof_region exec_op_stats =
	VACCEL_PROF_REGION_INIT("vaccel_exec_op");
struct vaccel_prof_region exec_res_op_stats =
	VACCEL_PROF_REGION_INIT("vaccel_exec_with_resource_op");

int vaccel_exec(struct vaccel_session *sess, const char *library,
		const char *fn_symbol, struct vaccel_arg *read, size_t nr_read,
		struct vaccel_arg *write, size_t nr_write)
{
	int ret;

	if (!sess)
		return VACCEL_EINVAL;

	vaccel_debug("session:%u Looking for plugin implementing exec",
		     sess->session_id);

	vaccel_prof_region_start(&exec_op_stats);

	// Get implementation
	int (*plugin_op)() = get_plugin_op(VACCEL_EXEC, sess->hint);
	if (!plugin_op)
		return VACCEL_ENOTSUP;

	ret = plugin_op(sess, library, fn_symbol, read, nr_read, write,
			nr_write);

	vaccel_prof_region_stop(&exec_op_stats);

	return ret;
}

int vaccel_exec_with_resource(struct vaccel_session *sess,
			      struct vaccel_shared_object *object,
			      const char *fn_symbol, struct vaccel_arg *read,
			      size_t nr_read, struct vaccel_arg *write,
			      size_t nr_write)
{
	int ret;

	if (!sess)
		return VACCEL_EINVAL;

	vaccel_debug(
		"session:%u Looking for plugin implementing exec with resource",
		sess->session_id);

	vaccel_prof_region_start(&exec_res_op_stats);

	// Get implementation
	int (*plugin_op)() =
		get_plugin_op(VACCEL_EXEC_WITH_RESOURCE, sess->hint);
	if (!plugin_op)
		return VACCEL_ENOTSUP;

	ret = plugin_op(sess, object, fn_symbol, read, nr_read, write,
			nr_write);

	vaccel_prof_region_stop(&exec_op_stats);

	return ret;
}

int vaccel_exec_unpack(struct vaccel_session *sess, struct vaccel_arg *read,
		       int nr_read, struct vaccel_arg *write, int nr_write)
{
	if (nr_read < 2) {
		vaccel_error("Wrong number of read arguments in exec: %d",
			     nr_read);
		return VACCEL_EINVAL;
	}

	/* Pop the first two arguments */
	char *library = (char *)read[0].buf;
	char *fn_symbol = (char *)read[1].buf;

	/* Pass on the rest of the read and all write arguments */
	return vaccel_exec(sess, library, fn_symbol, &read[2], nr_read - 2,
			   write, nr_write);
}

int vaccel_exec_with_res_unpack(struct vaccel_session *sess,
				struct vaccel_arg *read, int nr_read,
				struct vaccel_arg *write, int nr_write)
{
	int ret = VACCEL_EINVAL;
	if (nr_read < 2) {
		vaccel_error("Wrong number of read arguments in exec: %d",
			     nr_read);
		return VACCEL_EINVAL;
	}

	/* Pop the first two arguments */
	struct vaccel_resource *resource;
	struct vaccel_shared_object *object;

	ret = resource_get_by_id(&resource, *(long long int *)read[0].buf);
	if (ret) {
		vaccel_error("cannot find resource: %d", ret);
		return ret;
	}

	object = (struct vaccel_shared_object *)resource->data;
	if (!object) {
		vaccel_error("resource is empty..");
		return VACCEL_EINVAL;
	}

	char *fn_symbol = (char *)read[1].buf;

	/* Pass on the rest of the read and all write arguments */
	return vaccel_exec_with_resource(sess, object, fn_symbol, &read[2],
					 nr_read - 2, write, nr_write);
}

__attribute__((constructor)) static void vaccel_ops_init(void)
{
}

__attribute__((destructor)) static void vaccel_ops_fini(void)
{
	vaccel_prof_region_print(&exec_op_stats);
	vaccel_prof_region_print(&exec_res_op_stats);
}
