// SPDX-License-Identifier: Apache-2.0

#define _POSIX_C_SOURCE 200809L

#include "tflite.h"
#include "error.h"
#include "log.h"
#include "op.h"
#include "plugin.h"
#include "prof.h"
#include "resource.h"
#include "session.h"
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct vaccel_tflite_tensor *
vaccel_tflite_tensor_new(int nr_dims, int32_t *dims,
			 enum vaccel_tflite_type type)
{
	struct vaccel_tflite_tensor *ret;

	ret = calloc(1, sizeof(*ret));
	if (!ret)
		return NULL;

	ret->data_type = type;
	ret->nr_dims = nr_dims;
	ret->dims = calloc(nr_dims, sizeof(*dims));
	if (!ret->dims) {
		free(ret);
		return NULL;
	}

	if (dims)
		memcpy(ret->dims, dims, nr_dims * sizeof(*dims));

	ret->data = NULL;
	ret->size = 0;
	ret->owned = false;

	return ret;
}

struct vaccel_tflite_tensor *
vaccel_tflite_tensor_allocate(int nr_dims, int32_t *dims,
			      enum vaccel_tflite_type type, size_t total_size)
{
	struct vaccel_tflite_tensor *ret =
		vaccel_tflite_tensor_new(nr_dims, dims, type);
	if (!ret)
		return NULL;

	if (!total_size)
		return ret;

	ret->data = malloc(total_size);
	if (!ret)
		goto free_tensor;

	ret->size = total_size;
	ret->owned = true;

	return ret;

free_tensor:
	free(ret);
	return NULL;
}

int vaccel_tflite_tensor_destroy(struct vaccel_tflite_tensor *tensor)
{
	if (!tensor)
		return VACCEL_EINVAL;

	free(tensor->dims);

	if (tensor->data && tensor->owned)
		free(tensor->data);

	free(tensor);
	return VACCEL_OK;
}

int vaccel_tflite_tensor_set_data(struct vaccel_tflite_tensor *tensor,
				  void *data, size_t size)
{
	if (!tensor)
		return VACCEL_EINVAL;

	if (tensor->data && tensor->owned)
		free(tensor->data);

	tensor->data = data;
	tensor->size = size;
	tensor->owned = false;

	return VACCEL_OK;
}

void *vaccel_tflite_tensor_get_data(struct vaccel_tflite_tensor *tensor)
{
	if (!tensor)
		return NULL;

	return tensor->data;
}

struct vaccel_prof_region tflite_load_stats =
	VACCEL_PROF_REGION_INIT("vaccel_tflite_session_load");

typedef int (*tflite_session_load_fn_t)(struct vaccel_session *sess,
					struct vaccel_resource *model);

int vaccel_tflite_session_load(struct vaccel_session *sess,
			       struct vaccel_resource *model)
{
	int ret;

	if (!sess) {
		vaccel_debug("Invalid session");
		return VACCEL_EINVAL;
	}

	vaccel_debug(
		"session:%" PRId64
		" Looking for plugin implementing tflite_session_load operation",
		sess->id);

	if (model->type != VACCEL_RESOURCE_MODEL) {
		vaccel_error(
			"Invalid resource type: expected VACCEL_RESOURCE_MODEL");
		return VACCEL_EINVAL;
	}

	if (!vaccel_session_has_resource(sess, model)) {
		vaccel_error("Resource %" PRId64
			     " is not registered to session %" PRId64 "",
			     model->id, sess->id);
		return VACCEL_EPERM;
	}

	vaccel_prof_region_start(&tflite_load_stats);

	tflite_session_load_fn_t plugin_tflite_session_load =
		plugin_get_op_func(VACCEL_TFLITE_SESSION_LOAD, sess->hint);
	if (!plugin_tflite_session_load) {
		ret = VACCEL_ENOTSUP;
		goto out;
	}

	ret = plugin_tflite_session_load(sess, model);

out:
	vaccel_prof_region_stop(&tflite_load_stats);
	return ret;
}

struct vaccel_prof_region tflite_session_run_stats =
	VACCEL_PROF_REGION_INIT("vaccel_tflite_session_run");

typedef int (*tflite_session_run_fn_t)(struct vaccel_session *sess,
				       const struct vaccel_resource *model,
				       struct vaccel_tflite_tensor *const *in,
				       int nr_inputs,
				       struct vaccel_tflite_tensor **out,
				       int nr_outputs, uint8_t *status);

int vaccel_tflite_session_run(struct vaccel_session *sess,
			      const struct vaccel_resource *model,
			      struct vaccel_tflite_tensor *const *in,
			      int nr_inputs, struct vaccel_tflite_tensor **out,
			      int nr_outputs, uint8_t *status)
{
	int ret;

	if (!sess) {
		vaccel_debug("Invalid session");
		return VACCEL_EINVAL;
	}

	vaccel_debug(
		"session:%" PRId64
		" Looking for plugin implementing tflite_session_run operation",
		sess->id);

	if (model->type != VACCEL_RESOURCE_MODEL) {
		vaccel_error(
			"Invalid resource type: expected VACCEL_RESOURCE_MODEL");
		return VACCEL_EINVAL;
	}

	if (!vaccel_session_has_resource(sess, model)) {
		vaccel_error("Resource %" PRId64
			     " is not registered to session %" PRId64 "",
			     model->id, sess->id);
		return VACCEL_EPERM;
	}

	vaccel_prof_region_start(&tflite_session_run_stats);

	tflite_session_run_fn_t plugin_tflite_session =
		plugin_get_op_func(VACCEL_TFLITE_SESSION_RUN, sess->hint);
	if (!plugin_tflite_session) {
		ret = VACCEL_ENOTSUP;
		goto out;
	}

	ret = plugin_tflite_session(sess, model, in, nr_inputs, out, nr_outputs,
				    status);

out:
	vaccel_prof_region_stop(&tflite_session_run_stats);
	return ret;
}

struct vaccel_prof_region tflite_session_delete_stats =
	VACCEL_PROF_REGION_INIT("vaccel_tflite_session_delete");

typedef int (*tflite_session_delete_fn_t)(struct vaccel_session *sess,
					  struct vaccel_resource *model);

int vaccel_tflite_session_delete(struct vaccel_session *sess,
				 struct vaccel_resource *model)
{
	int ret;

	if (!sess) {
		vaccel_debug("Invalid vAccel session");
		return VACCEL_EINVAL;
	}

	vaccel_debug(
		"session:%" PRId64
		" Looking for plugin implementing tflite_session_delete operation",
		sess->id);

	if (model->type != VACCEL_RESOURCE_MODEL) {
		vaccel_error(
			"Invalid resource type: expected VACCEL_RESOURCE_MODEL");
		return VACCEL_EINVAL;
	}

	if (!vaccel_session_has_resource(sess, model)) {
		vaccel_error("Resource %" PRId64
			     " is not registered to session %" PRId64 "",
			     model->id, sess->id);
		return VACCEL_EPERM;
	}

	tflite_session_delete_fn_t plugin_tflite_session_delete =
		plugin_get_op_func(VACCEL_TFLITE_SESSION_DELETE, sess->hint);
	if (!plugin_tflite_session_delete) {
		ret = VACCEL_ENOTSUP;
		goto out;
	}

	ret = plugin_tflite_session_delete(sess, model);

out:
	vaccel_prof_region_stop(&tflite_session_delete_stats);
	return ret;
}

__attribute__((constructor)) static void vaccel_tflite_ops_init(void)
{
}

__attribute__((destructor)) static void vaccel_tflite_ops_fini(void)
{
	vaccel_prof_region_print(&tflite_load_stats);
	vaccel_prof_region_print(&tflite_session_run_stats);
	vaccel_prof_region_print(&tflite_session_delete_stats);
}
