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

static struct vaccel_prof_region exec_op_stats =
	VACCEL_PROF_REGION_INIT("vaccel_exec_op");

typedef int (*exec_fn_t)(struct vaccel_session *sess, const char *library,
			 const char *fn_symbol, struct vaccel_arg *read,
			 size_t nr_read, struct vaccel_arg *write,
			 size_t nr_write);

int vaccel_exec(struct vaccel_session *sess, const char *library,
		const char *fn_symbol, struct vaccel_arg *read, size_t nr_read,
		struct vaccel_arg *write, size_t nr_write)
{
	int ret;

	if (!sess)
		return VACCEL_EINVAL;

	vaccel_op_type_t op_type = VACCEL_OP_EXEC;
	op_debug_plugin_lookup(sess, op_type);

	vaccel_prof_region_start(&exec_op_stats);

	exec_fn_t plugin_exec = plugin_get_op_func(sess->plugin, op_type);
	if (!plugin_exec) {
		ret = VACCEL_ENOTSUP;
		goto out;
	}

	ret = plugin_exec(sess, library, fn_symbol, read, nr_read, write,
			  nr_write);

out:
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

	struct vaccel_arg_array read_args;
	int ret = vaccel_arg_array_wrap(&read_args, read, nr_read);
	if (ret) {
		vaccel_error("Failed to parse exec read args");
		return VACCEL_EINVAL;
	}

	/* Pop the first two arguments */
	char *library;
	ret = vaccel_arg_array_get_string(&read_args, &library);
	if (ret) {
		vaccel_error("Failed to unpack library string for exec");
		return VACCEL_EINVAL;
	}

	char *fn_symbol;
	ret = vaccel_arg_array_get_string(&read_args, &fn_symbol);
	if (ret) {
		vaccel_error("Failed to unpack function symbol for exec");
		return VACCEL_EINVAL;
	}

	/* Pass on the rest of the read and all write arguments */
	struct vaccel_arg *exec_read;
	size_t exec_nr_read;
	ret = vaccel_arg_array_get_remaining(&read_args, &exec_read,
					     &exec_nr_read);
	if (ret) {
		vaccel_error("Failed to get exec read args");
		return VACCEL_EINVAL;
	}

	return vaccel_exec(sess, library, fn_symbol, exec_read, exec_nr_read,
			   write, nr_write);
}

static struct vaccel_prof_region exec_res_op_stats =
	VACCEL_PROF_REGION_INIT("vaccel_exec_with_resource_op");

typedef int (*exec_with_resource_fn_t)(struct vaccel_session *sess,
				       struct vaccel_resource *resource,
				       const char *fn_symbol,
				       struct vaccel_arg *read, size_t nr_read,
				       struct vaccel_arg *write,
				       size_t nr_write);

int vaccel_exec_with_resource(struct vaccel_session *sess,
			      struct vaccel_resource *resource,
			      const char *fn_symbol, struct vaccel_arg *read,
			      size_t nr_read, struct vaccel_arg *write,
			      size_t nr_write)
{
	int ret;

	if (!sess || !resource)
		return VACCEL_EINVAL;

	vaccel_op_type_t op_type = VACCEL_OP_EXEC_WITH_RESOURCE;
	op_debug_plugin_lookup(sess, op_type);

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

	exec_with_resource_fn_t plugin_exec_with_resource =
		plugin_get_op_func(sess->plugin, op_type);
	if (!plugin_exec_with_resource) {
		ret = VACCEL_ENOTSUP;
		goto out;
	}

	ret = plugin_exec_with_resource(sess, resource, fn_symbol, read,
					nr_read, write, nr_write);

out:
	vaccel_prof_region_stop(&exec_res_op_stats);

	return ret;
}

int vaccel_exec_with_res_unpack(struct vaccel_session *sess,
				struct vaccel_arg *read, int nr_read,
				struct vaccel_arg *write, int nr_write)
{
	if (nr_read < 2) {
		vaccel_error(
			"Wrong number of read arguments in exec_with_resource: %d",
			nr_read);
		return VACCEL_EINVAL;
	}

	struct vaccel_arg_array read_args;
	int ret = vaccel_arg_array_wrap(&read_args, read, nr_read);
	if (ret) {
		vaccel_error("Failed to parse exec_with_resource read args");
		return VACCEL_EINVAL;
	}

	/* Pop the first two arguments */
	vaccel_id_t res_id;
	ret = vaccel_arg_array_get_int64(&read_args, &res_id);
	if (ret) {
		vaccel_error(
			"Failed to unpack library resource id for exec_with_resource");
		return VACCEL_EINVAL;
	}

	char *fn_symbol;
	ret = vaccel_arg_array_get_string(&read_args, &fn_symbol);
	if (ret) {
		vaccel_error(
			"Failed to unpack function symbol for exec_with_resource");
		return VACCEL_EINVAL;
	}

	/* Get library resource */
	struct vaccel_resource *resource;
	ret = vaccel_resource_get_by_id(&resource, res_id);
	if (ret) {
		vaccel_error(
			"Could not find exec_with_resource library resource");
		return ret;
	}

	/* Pass on the rest of the read and all write arguments */
	struct vaccel_arg *exec_read;
	size_t exec_nr_read;
	ret = vaccel_arg_array_get_remaining(&read_args, &exec_read,
					     &exec_nr_read);
	if (ret) {
		vaccel_error("Failed to get exec_with_resource read args");
		return VACCEL_EINVAL;
	}

	return vaccel_exec_with_resource(sess, resource, fn_symbol, exec_read,
					 exec_nr_read, write, nr_write);
}

__attribute__((constructor)) static void vaccel_ops_init(void)
{
}

__attribute__((destructor)) static void vaccel_ops_fini(void)
{
	vaccel_prof_region_print(&exec_op_stats);
	vaccel_prof_region_print(&exec_res_op_stats);

	vaccel_prof_region_release(&exec_op_stats);
	vaccel_prof_region_release(&exec_res_op_stats);
}
