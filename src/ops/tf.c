// SPDX-License-Identifier: Apache-2.0

#define _POSIX_C_SOURCE 200809L

#include "tf.h"
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

int vaccel_tf_buffer_init(struct vaccel_tf_buffer *buffer, void *data,
			  size_t size)
{
	if (!buffer)
		return VACCEL_EINVAL;

	buffer->data = data;
	buffer->size = size;

	return VACCEL_OK;
}

int vaccel_tf_buffer_release(struct vaccel_tf_buffer *buffer)
{
	if (!buffer)
		return VACCEL_EINVAL;

	if (buffer->data)
		free(buffer->data);

	buffer->data = NULL;
	buffer->size = 0;

	return VACCEL_OK;
}

int vaccel_tf_buffer_new(struct vaccel_tf_buffer **buffer, void *data,
			 size_t size)
{
	if (!buffer)
		return VACCEL_EINVAL;

	struct vaccel_tf_buffer *b = calloc(1, sizeof(*b));
	if (!b)
		return VACCEL_ENOMEM;

	int ret = vaccel_tf_buffer_init(b, data, size);
	if (ret) {
		free(b);
		return ret;
	}

	*buffer = b;

	return VACCEL_OK;
}

int vaccel_tf_buffer_delete(struct vaccel_tf_buffer *buffer)
{
	int ret = vaccel_tf_buffer_release(buffer);
	if (ret)
		return ret;

	free(buffer);

	return VACCEL_OK;
}

int vaccel_tf_buffer_take_data(struct vaccel_tf_buffer *buffer, void **data,
			       size_t *size)
{
	if (!buffer || !data || !size)
		return VACCEL_EINVAL;

	*data = buffer->data;
	*size = buffer->size;

	buffer->data = NULL;
	buffer->size = 0;

	return VACCEL_OK;
}

int vaccel_tf_node_init(struct vaccel_tf_node *node, const char *name, int id)
{
	if (!node || !name)
		return VACCEL_EINVAL;

	node->name = strdup(name);
	if (!node->name)
		return VACCEL_ENOMEM;
	node->id = id;

	return VACCEL_OK;
}

int vaccel_tf_node_release(struct vaccel_tf_node *node)
{
	if (!node)
		return VACCEL_EINVAL;

	if (node->name)
		free(node->name);

	node->name = NULL;
	node->id = -1;

	return VACCEL_OK;
}

int vaccel_tf_node_new(struct vaccel_tf_node **node, const char *name, int id)
{
	if (!node)
		return VACCEL_EINVAL;

	struct vaccel_tf_node *n = malloc(sizeof(*n));
	if (!n)
		return VACCEL_ENOMEM;

	int ret = vaccel_tf_node_init(n, name, id);
	if (ret) {
		free(n);
		return ret;
	}

	*node = n;

	return VACCEL_OK;
}

int vaccel_tf_node_delete(struct vaccel_tf_node *node)
{
	int ret = vaccel_tf_node_release(node);
	if (ret)
		return ret;

	free(node);

	return VACCEL_OK;
}

int vaccel_tf_tensor_init(struct vaccel_tf_tensor *tensor, int nr_dims,
			  const int64_t *dims, enum vaccel_tf_data_type type)
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

int vaccel_tf_tensor_release(struct vaccel_tf_tensor *tensor)
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

int vaccel_tf_tensor_new(struct vaccel_tf_tensor **tensor, int nr_dims,
			 const int64_t *dims, enum vaccel_tf_data_type type)
{
	if (!tensor)
		return VACCEL_EINVAL;

	struct vaccel_tf_tensor *t = calloc(1, sizeof(*t));
	if (!t)
		return VACCEL_ENOMEM;

	int ret = vaccel_tf_tensor_init(t, nr_dims, dims, type);
	if (ret) {
		free(t);
		return ret;
	}

	*tensor = t;

	return VACCEL_OK;
}

int vaccel_tf_tensor_allocate(struct vaccel_tf_tensor **tensor, int nr_dims,
			      const int64_t *dims,
			      enum vaccel_tf_data_type type, size_t total_size)
{
	int ret = vaccel_tf_tensor_new(tensor, nr_dims, dims, type);
	if (ret)
		return ret;

	if (!total_size)
		return VACCEL_OK;

	(*tensor)->data = malloc(total_size);
	if (!(*tensor)->data) {
		vaccel_tf_tensor_delete(*tensor);
		return VACCEL_ENOMEM;
	}

	(*tensor)->size = total_size;
	(*tensor)->owned = true;

	return VACCEL_OK;
}

int vaccel_tf_tensor_delete(struct vaccel_tf_tensor *tensor)
{
	int ret = vaccel_tf_tensor_release(tensor);
	if (ret)
		return ret;

	free(tensor);

	return VACCEL_OK;
}

int vaccel_tf_tensor_set_data(struct vaccel_tf_tensor *tensor, void *data,
			      size_t size)
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

int vaccel_tf_tensor_take_data(struct vaccel_tf_tensor *tensor, void **data,
			       size_t *size)
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

int vaccel_tf_status_init(struct vaccel_tf_status *status, uint8_t error_code,
			  const char *message)
{
	if (!status || !message)
		return VACCEL_EINVAL;

	status->error_code = error_code;
	status->message = strdup(message);
	if (!status->message)
		return VACCEL_ENOMEM;

	return VACCEL_OK;
}

int vaccel_tf_status_release(struct vaccel_tf_status *status)
{
	if (!status)
		return VACCEL_EINVAL;

	if (status->message)
		free(status->message);

	status->error_code = 0;
	status->message = NULL;

	return VACCEL_OK;
}

int vaccel_tf_status_new(struct vaccel_tf_status **status, uint8_t error_code,
			 const char *message)
{
	if (!status)
		return VACCEL_EINVAL;

	struct vaccel_tf_status *s = calloc(1, sizeof(*s));
	if (!s)
		return VACCEL_ENOMEM;

	int ret = vaccel_tf_status_init(s, error_code, message);
	if (ret) {
		free(s);
		return ret;
	}

	*status = s;

	return VACCEL_OK;
}

int vaccel_tf_status_delete(struct vaccel_tf_status *status)
{
	int ret = vaccel_tf_status_release(status);
	if (ret)
		return ret;

	free(status);

	return VACCEL_OK;
}

static struct vaccel_prof_region tf_session_load_op_stats =
	VACCEL_PROF_REGION_INIT("vaccel_tf_session_load");

typedef int (*tf_session_load_fn_t)(struct vaccel_session *sess,
				    struct vaccel_resource *model,
				    struct vaccel_tf_status *status);

int vaccel_tf_session_load(struct vaccel_session *sess,
			   struct vaccel_resource *model,
			   struct vaccel_tf_status *status)
{
	int ret;

	if (!sess) {
		vaccel_debug("Invalid session");
		return VACCEL_EINVAL;
	}

	vaccel_debug(
		"session:%" PRId64
		" Looking for plugin implementing tf_session_load operation",
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

	vaccel_prof_region_start(&tf_session_load_op_stats);

	tf_session_load_fn_t plugin_tf_session_load =
		plugin_get_op_func(VACCEL_OP_TF_SESSION_LOAD, sess->hint);
	if (!plugin_tf_session_load) {
		ret = VACCEL_ENOTSUP;
		goto out;
	}

	ret = plugin_tf_session_load(sess, model, status);

out:
	vaccel_prof_region_stop(&tf_session_load_op_stats);

	return ret;
}

static struct vaccel_prof_region tf_session_run_op_stats =
	VACCEL_PROF_REGION_INIT("vaccel_tf_session_run");

typedef int (*tf_session_run_fn_t)(
	struct vaccel_session *sess, const struct vaccel_resource *model,
	const struct vaccel_tf_buffer *run_options,
	const struct vaccel_tf_node *in_nodes,
	struct vaccel_tf_tensor *const *in, int nr_inputs,
	const struct vaccel_tf_node *out_nodes, struct vaccel_tf_tensor **out,
	int nr_outputs, struct vaccel_tf_status *status);

int vaccel_tf_session_run(struct vaccel_session *sess,
			  const struct vaccel_resource *model,
			  const struct vaccel_tf_buffer *run_options,
			  const struct vaccel_tf_node *in_nodes,
			  struct vaccel_tf_tensor *const *in, int nr_inputs,
			  const struct vaccel_tf_node *out_nodes,
			  struct vaccel_tf_tensor **out, int nr_outputs,
			  struct vaccel_tf_status *status)
{
	int ret;

	if (!sess) {
		vaccel_debug("Invalid session");
		return VACCEL_EINVAL;
	}

	vaccel_debug(
		"session:%u Looking for plugin implementing tf_session_run operation",
		sess->id);

	if (model->type != VACCEL_RESOURCE_MODEL) {
		vaccel_error(
			"Invalid resource type: expected VACCEL_RESOURCE_MODEL");
		return VACCEL_EINVAL;
	}

	if (!vaccel_session_has_resource(sess, model)) {
		vaccel_error("Resource %u is not registered to session %u",
			     model->id, sess->id);
		return VACCEL_EPERM;
	}

	vaccel_prof_region_start(&tf_session_run_op_stats);

	tf_session_run_fn_t plugin_tf_session_run =
		plugin_get_op_func(VACCEL_OP_TF_SESSION_RUN, sess->hint);
	if (!plugin_tf_session_run) {
		ret = VACCEL_ENOTSUP;
		goto out;
	}

	ret = plugin_tf_session_run(sess, model, run_options, in_nodes, in,
				    nr_inputs, out_nodes, out, nr_outputs,
				    status);

out:
	vaccel_prof_region_stop(&tf_session_run_op_stats);

	return ret;
}

static struct vaccel_prof_region tf_session_delete_op_stats =
	VACCEL_PROF_REGION_INIT("vaccel_tf_session_delete");

typedef int (*tf_session_delete_fn_t)(struct vaccel_session *sess,
				      struct vaccel_resource *model,
				      struct vaccel_tf_status *status);

int vaccel_tf_session_delete(struct vaccel_session *sess,
			     struct vaccel_resource *model,
			     struct vaccel_tf_status *status)
{
	int ret;

	if (!sess) {
		vaccel_debug("Invalid vAccel session");
		return VACCEL_EINVAL;
	}

	vaccel_debug(
		"session:%u Looking for plugin implementing tf_session_delete operation",
		sess->id);

	if (model->type != VACCEL_RESOURCE_MODEL) {
		vaccel_error(
			"Invalid resource type: expected VACCEL_RESOURCE_MODEL");
		return VACCEL_EINVAL;
	}

	if (!vaccel_session_has_resource(sess, model)) {
		vaccel_error("Resource %u is not registered to session %u",
			     model->id, sess->id);
		return VACCEL_EPERM;
	}

	vaccel_prof_region_start(&tf_session_delete_op_stats);

	tf_session_delete_fn_t plugin_tf_session_delete =
		plugin_get_op_func(VACCEL_OP_TF_SESSION_DELETE, sess->hint);
	if (!plugin_tf_session_delete) {
		ret = VACCEL_ENOTSUP;
		goto out;
	}

	ret = plugin_tf_session_delete(sess, model, status);

out:
	vaccel_prof_region_stop(&tf_session_delete_op_stats);

	return ret;
}

__attribute__((constructor)) static void vaccel_tf_ops_init(void)
{
}

__attribute__((destructor)) static void vaccel_tf_ops_fini(void)
{
	vaccel_prof_region_print(&tf_session_load_op_stats);
	vaccel_prof_region_release(&tf_session_load_op_stats);

	vaccel_prof_region_print(&tf_session_run_op_stats);
	vaccel_prof_region_release(&tf_session_run_op_stats);

	vaccel_prof_region_print(&tf_session_delete_op_stats);
	vaccel_prof_region_release(&tf_session_delete_op_stats);
}
