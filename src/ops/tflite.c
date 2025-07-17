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

int vaccel_tflite_tensor_init(struct vaccel_tflite_tensor *tensor, int nr_dims,
			      const int32_t *dims,
			      enum vaccel_tflite_data_type type)
{
	if (!tensor || nr_dims < 1)
		return VACCEL_EINVAL;

	tensor->data_type = type;
	tensor->data = NULL;
	tensor->size = 0;
	tensor->owned = false;
	tensor->nr_dims = nr_dims;
	tensor->dims = calloc(nr_dims, sizeof(*dims));
	if (!tensor->dims)
		return VACCEL_ENOMEM;

	if (dims) {
		for (int i = 0; i < nr_dims; i++) {
			tensor->dims[i] = dims[i];
		}
	}

	return VACCEL_OK;
}

int vaccel_tflite_tensor_release(struct vaccel_tflite_tensor *tensor)
{
	if (!tensor)
		return VACCEL_EINVAL;

	free(tensor->dims);

	if (tensor->data && tensor->owned)
		free(tensor->data);

	tensor->data = NULL;
	tensor->size = 0;
	tensor->owned = false;
	tensor->nr_dims = 0;
	tensor->dims = NULL;

	return VACCEL_OK;
}

int vaccel_tflite_tensor_new(struct vaccel_tflite_tensor **tensor, int nr_dims,
			     const int32_t *dims,
			     enum vaccel_tflite_data_type type)
{
	if (!tensor)
		return VACCEL_EINVAL;

	struct vaccel_tflite_tensor *t = calloc(1, sizeof(*t));
	if (!t)
		return VACCEL_ENOMEM;

	int ret = vaccel_tflite_tensor_init(t, nr_dims, dims, type);
	if (ret) {
		free(t);
		return ret;
	}

	*tensor = t;

	return VACCEL_OK;
}

int vaccel_tflite_tensor_allocate(struct vaccel_tflite_tensor **tensor,
				  int nr_dims, const int32_t *dims,
				  enum vaccel_tflite_data_type type,
				  size_t total_size)
{
	int ret = vaccel_tflite_tensor_new(tensor, nr_dims, dims, type);
	if (ret)
		return ret;

	if (!total_size)
		return VACCEL_OK;

	(*tensor)->data = malloc(total_size);
	if (!(*tensor)->data) {
		vaccel_tflite_tensor_release(*tensor);
		return VACCEL_ENOMEM;
	}

	(*tensor)->size = total_size;
	(*tensor)->owned = true;

	return VACCEL_OK;
}

int vaccel_tflite_tensor_delete(struct vaccel_tflite_tensor *tensor)
{
	int ret = vaccel_tflite_tensor_release(tensor);
	if (ret)
		return ret;

	free(tensor);

	return VACCEL_OK;
}

int vaccel_tflite_tensor_set_data(struct vaccel_tflite_tensor *tensor,
				  void *data, size_t size)
{
	if (!tensor)
		return VACCEL_EINVAL;

	if (tensor->data && tensor->owned)
		vaccel_warn(
			"Previous tensor data will not be freed by release");

	tensor->data = data;
	tensor->size = size;
	tensor->owned = false;

	return VACCEL_OK;
}

int vaccel_tflite_tensor_take_data(struct vaccel_tflite_tensor *tensor,
				   void **data, size_t *size)
{
	if (!tensor || !data || !size)
		return VACCEL_EINVAL;

	*data = tensor->data;
	*size = tensor->size;

	tensor->data = NULL;
	tensor->size = 0;
	tensor->owned = false;

	return VACCEL_OK;
}

static struct vaccel_prof_region tflite_model_load_op_stats =
	VACCEL_PROF_REGION_INIT("vaccel_tflite_model_load");

typedef int (*tflite_model_load_fn_t)(struct vaccel_session *sess,
				      struct vaccel_resource *model);

int vaccel_tflite_model_load(struct vaccel_session *sess,
			     struct vaccel_resource *model)
{
	int ret;

	if (!sess || !model)
		return VACCEL_EINVAL;

	vaccel_op_type_t op_type = VACCEL_OP_TFLITE_MODEL_LOAD;
	op_debug_plugin_lookup(sess, op_type);

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

	vaccel_prof_region_start(&tflite_model_load_op_stats);

	tflite_model_load_fn_t plugin_tflite_model_load =
		plugin_get_op_func(op_type, sess->hint);
	if (!plugin_tflite_model_load) {
		ret = VACCEL_ENOTSUP;
		goto out;
	}

	ret = plugin_tflite_model_load(sess, model);

out:
	vaccel_prof_region_stop(&tflite_model_load_op_stats);

	return ret;
}

static struct vaccel_prof_region tflite_model_unload_op_stats =
	VACCEL_PROF_REGION_INIT("vaccel_tflite_model_unload");

typedef int (*tflite_model_unload_fn_t)(struct vaccel_session *sess,
					struct vaccel_resource *model);

int vaccel_tflite_model_unload(struct vaccel_session *sess,
			       struct vaccel_resource *model)
{
	int ret;

	if (!sess || !model)
		return VACCEL_EINVAL;

	vaccel_op_type_t op_type = VACCEL_OP_TFLITE_MODEL_UNLOAD;
	op_debug_plugin_lookup(sess, op_type);

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

	vaccel_prof_region_start(&tflite_model_unload_op_stats);

	tflite_model_unload_fn_t plugin_tflite_model_unload =
		plugin_get_op_func(op_type, sess->hint);
	if (!plugin_tflite_model_unload) {
		ret = VACCEL_ENOTSUP;
		goto out;
	}

	ret = plugin_tflite_model_unload(sess, model);

out:
	vaccel_prof_region_stop(&tflite_model_unload_op_stats);

	return ret;
}

static struct vaccel_prof_region tflite_model_run_op_stats =
	VACCEL_PROF_REGION_INIT("vaccel_tflite_model_run");

typedef int (*tflite_model_run_fn_t)(struct vaccel_session *sess,
				     const struct vaccel_resource *model,
				     struct vaccel_tflite_tensor *const *in,
				     int nr_inputs,
				     struct vaccel_tflite_tensor **out,
				     int nr_outputs, uint8_t *status);

int vaccel_tflite_model_run(struct vaccel_session *sess,
			    const struct vaccel_resource *model,
			    struct vaccel_tflite_tensor *const *in,
			    int nr_inputs, struct vaccel_tflite_tensor **out,
			    int nr_outputs, uint8_t *status)
{
	int ret;

	if (!sess || !model)
		return VACCEL_EINVAL;

	vaccel_op_type_t op_type = VACCEL_OP_TFLITE_MODEL_RUN;
	op_debug_plugin_lookup(sess, op_type);

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

	vaccel_prof_region_start(&tflite_model_run_op_stats);

	tflite_model_run_fn_t plugin_tflite_model =
		plugin_get_op_func(op_type, sess->hint);
	if (!plugin_tflite_model) {
		ret = VACCEL_ENOTSUP;
		goto out;
	}

	ret = plugin_tflite_model(sess, model, in, nr_inputs, out, nr_outputs,
				  status);

out:
	vaccel_prof_region_stop(&tflite_model_run_op_stats);

	return ret;
}

__attribute__((constructor)) static void vaccel_tflite_ops_init(void)
{
}

__attribute__((destructor)) static void vaccel_tflite_ops_fini(void)
{
	vaccel_prof_region_print(&tflite_model_load_op_stats);
	vaccel_prof_region_print(&tflite_model_unload_op_stats);
	vaccel_prof_region_print(&tflite_model_run_op_stats);

	vaccel_prof_region_release(&tflite_model_load_op_stats);
	vaccel_prof_region_release(&tflite_model_unload_op_stats);
	vaccel_prof_region_release(&tflite_model_run_op_stats);
}
