// SPDX-License-Identifier: Apache-2.0

#include "torch.h"
#include "error.h"
#include "log.h"
#include "op.h"
#include "plugin.h"
#include "resource.h"
#include "session.h"
#include <inttypes.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

int vaccel_torch_buffer_init(struct vaccel_torch_buffer *buffer, char *data,
			     size_t size)
{
	if (!buffer)
		return VACCEL_EINVAL;

	buffer->data = data;
	buffer->size = size;

	return VACCEL_OK;
}

int vaccel_torch_buffer_release(struct vaccel_torch_buffer *buffer)
{
	if (!buffer)
		return VACCEL_EINVAL;

	if (buffer->data)
		free(buffer->data);

	buffer->data = NULL;
	buffer->size = 0;

	return VACCEL_OK;
}

int vaccel_torch_buffer_new(struct vaccel_torch_buffer **buffer, char *data,
			    size_t size)
{
	if (!buffer)
		return VACCEL_EINVAL;

	struct vaccel_torch_buffer *b = calloc(1, sizeof(*b));
	if (!b)
		return VACCEL_ENOMEM;

	int ret = vaccel_torch_buffer_init(b, data, size);
	if (ret) {
		free(b);
		return ret;
	}

	*buffer = b;

	return VACCEL_OK;
}

int vaccel_torch_buffer_delete(struct vaccel_torch_buffer *buffer)
{
	int ret = vaccel_torch_buffer_release(buffer);
	if (ret)
		return ret;

	free(buffer);

	return VACCEL_OK;
}

int vaccel_torch_buffer_take_data(struct vaccel_torch_buffer *buffer,
				  void **data, size_t *size)
{
	if (!buffer || !data || !size)
		return VACCEL_EINVAL;

	*data = buffer->data;
	*size = buffer->size;

	buffer->data = NULL;
	buffer->size = 0;

	return VACCEL_OK;
}

int vaccel_torch_tensor_init(struct vaccel_torch_tensor *tensor,
			     int64_t nr_dims, const int64_t *dims,
			     enum vaccel_torch_data_type type)
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
		for (int64_t i = 0; i < nr_dims; i++) {
			tensor->dims[i] = dims[i];
		}
	}

	return VACCEL_OK;
}

int vaccel_torch_tensor_release(struct vaccel_torch_tensor *tensor)
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

int vaccel_torch_tensor_new(struct vaccel_torch_tensor **tensor,
			    int64_t nr_dims, const int64_t *dims,
			    enum vaccel_torch_data_type type)
{
	if (!tensor)
		return VACCEL_EINVAL;

	struct vaccel_torch_tensor *t = calloc(1, sizeof(*t));
	if (!t)
		return VACCEL_EINVAL;

	int ret = vaccel_torch_tensor_init(t, nr_dims, dims, type);
	if (ret) {
		free(t);
		return ret;
	}

	*tensor = t;

	return VACCEL_OK;
}

int vaccel_torch_tensor_allocate(struct vaccel_torch_tensor **tensor,
				 int64_t nr_dims, const int64_t *dims,
				 enum vaccel_torch_data_type type,
				 size_t total_size)
{
	int ret = vaccel_torch_tensor_new(tensor, nr_dims, dims, type);
	if (ret)
		return ret;

	if (!total_size)
		return VACCEL_OK;

	(*tensor)->data = malloc(total_size);
	if (!(*tensor)->data) {
		vaccel_torch_tensor_delete(*tensor);
		return VACCEL_ENOMEM;
	}

	(*tensor)->size = total_size;
	(*tensor)->owned = true;

	return VACCEL_OK;
}

int vaccel_torch_tensor_delete(struct vaccel_torch_tensor *tensor)
{
	int ret = vaccel_torch_tensor_release(tensor);
	if (ret)
		return ret;

	free(tensor);

	return VACCEL_OK;
}

int vaccel_torch_tensor_set_data(struct vaccel_torch_tensor *tensor, void *data,
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

int vaccel_torch_tensor_take_data(struct vaccel_torch_tensor *tensor,
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

typedef int (*torch_jitload_forward_fn_t)(
	struct vaccel_session *sess, const struct vaccel_resource *model,
	const struct vaccel_torch_buffer *run_options,
	struct vaccel_torch_tensor **in_tensor, int nr_read,
	struct vaccel_torch_tensor **out_tensor, int nr_write);

int vaccel_torch_jitload_forward(struct vaccel_session *sess,
				 const struct vaccel_resource *model,
				 const struct vaccel_torch_buffer *run_options,
				 struct vaccel_torch_tensor **in_tensor,
				 int nr_read,
				 struct vaccel_torch_tensor **out_tensor,
				 int nr_write)
{
	if (!sess)
		return VACCEL_EINVAL;

	vaccel_debug(
		"session:%" PRId64
		" Looking for plugin implementing torch_jitload_forward operation",
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

	torch_jitload_forward_fn_t plugin_torch_jitload_forward =
		plugin_get_op_func(VACCEL_OP_TORCH_JITLOAD_FORWARD, sess->hint);
	if (!plugin_torch_jitload_forward)
		return VACCEL_ENOTSUP;

	return plugin_torch_jitload_forward(sess, model, run_options, in_tensor,
					    nr_read, out_tensor, nr_write);
}

typedef int (*torch_sgemm_fn_t)(struct vaccel_session *sess,
				struct vaccel_torch_tensor **in_A,
				struct vaccel_torch_tensor **in_B,
				struct vaccel_torch_tensor **in_C, int M, int N,
				int K, struct vaccel_torch_tensor **out);

int vaccel_torch_sgemm(struct vaccel_session *sess,
		       struct vaccel_torch_tensor **in_A,
		       struct vaccel_torch_tensor **in_B,
		       struct vaccel_torch_tensor **in_C, int M, int N, int K,
		       struct vaccel_torch_tensor **out)
{
	if (!sess)
		return VACCEL_EINVAL;

	vaccel_debug("session:%" PRId64
		     " Looking for plugin implementing torch sgemm operation",
		     sess->id);

	torch_sgemm_fn_t plugin_torch_sgemm =
		plugin_get_op_func(VACCEL_OP_TORCH_SGEMM, sess->hint);
	if (!plugin_torch_sgemm) {
		vaccel_debug("Plugin loading failed");
		return VACCEL_ENOTSUP;
	}

	return plugin_torch_sgemm(sess, in_A, in_B, in_C, M, N, K, out);
}
