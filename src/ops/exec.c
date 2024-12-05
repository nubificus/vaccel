// SPDX-License-Identifier: Apache-2.0

#include "exec.h"
#include "arg.h"
#include "error.h"
#include "id_pool.h"
#include "log.h"
#include "op.h"
#include "plugin.h"
#include "prof.h"
#include "resource.h"
#include "session.h"
#include <inttypes.h>
#include <stdint.h>

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

	vaccel_debug("session:%" PRId64 " Looking for plugin implementing exec",
		     sess->id);

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
			      struct vaccel_resource *resource,
			      const char *fn_symbol, struct vaccel_arg *read,
			      size_t nr_read, struct vaccel_arg *write,
			      size_t nr_write)
{
	int ret;

	if (!sess || !resource)
		return VACCEL_EINVAL;

	vaccel_debug("session:%" PRId64
		     " Looking for plugin implementing exec with resource",
		     sess->id);

	if (resource->type != VACCEL_RESOURCE_LIB) {
		vaccel_error(
			"Invalid resource type: expected VACCEL_RESOURCE_LIB");
		return VACCEL_EINVAL;
	}

	if (!vaccel_session_has_resource(sess, resource)) {
		vaccel_error("Resource %" PRId64
			     " is not registered to session %" PRId64 "",
			     resource->id, sess->id);
		return VACCEL_EPERM;
	}

	vaccel_prof_region_start(&exec_res_op_stats);

	// Get implementation
	int (*plugin_op)() =
		get_plugin_op(VACCEL_EXEC_WITH_RESOURCE, sess->hint);
	if (!plugin_op)
		return VACCEL_ENOTSUP;

	ret = plugin_op(sess, resource, fn_symbol, read, nr_read, write,
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

	ret = vaccel_resource_get_by_id(&resource, *(vaccel_id_t *)read[0].buf);
	if (ret) {
		vaccel_error("cannot find resource: %d", ret);
		return ret;
	}

	char *fn_symbol = (char *)read[1].buf;

	/* Pass on the rest of the read and all write arguments */
	return vaccel_exec_with_resource(sess, resource, fn_symbol, &read[2],
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
