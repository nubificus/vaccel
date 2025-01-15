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

// Torch buffer creation, includes: [char* image, size_t size]
// if set, during destruction calling `free`
struct vaccel_torch_buffer *vaccel_torch_buffer_new(char *data, size_t size)
{
	struct vaccel_torch_buffer *ret = calloc(1, sizeof(*ret));
	if (!ret)
		return NULL;

	ret->data = data;
	ret->size = size;

	return ret;
}

// Destroy Torch buffer data
void vaccel_torch_buffer_destroy(struct vaccel_torch_buffer *buffer)
{
	if (buffer->data)
		free(buffer->data);
	free(buffer);
}

void *vaccel_torch_buffer_take_data(struct vaccel_torch_buffer *buffer,
				    size_t *size)
{
	void *ptr;
	if (!buffer)
		return NULL;

	*size = buffer->size;
	ptr = buffer->data;
	buffer->data = NULL;
	buffer->size = 0;

	return ptr;
}

// TODO: Not a read-only pointer for buffer->data
void *vaccel_torch_buffer_get_data(struct vaccel_torch_buffer *buffer,
				   size_t *size)
{
	if (!buffer)
		return NULL;

	*size = buffer->size;
	return buffer->data;
}

struct vaccel_torch_tensor *
vaccel_torch_tensor_new(int64_t nr_dims, const int64_t *dims,
			enum vaccel_torch_data_type type)
{
	struct vaccel_torch_tensor *ret;

	ret = calloc(1, sizeof(*ret));
	if (!ret)
		return NULL;

	ret->data_type = type;
	ret->nr_dims = nr_dims;
	ret->dims = calloc(nr_dims, sizeof(*ret->dims));
	if (!ret->dims) {
		free(ret);
		return NULL;
	}

	if (dims) {
		for (int64_t i = 0; i < nr_dims; i++) {
			ret->dims[i] = dims[i];
		}
	}

	return ret;
}

struct vaccel_torch_tensor *
vaccel_torch_tensor_allocate(int64_t nr_dims, int64_t *dims,
			     enum vaccel_torch_data_type type,
			     size_t total_size)
{
	struct vaccel_torch_tensor *ret =
		vaccel_torch_tensor_new(nr_dims, dims, type);
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

int vaccel_torch_tensor_destroy(struct vaccel_torch_tensor *tensor)
{
	if (!tensor)
		return VACCEL_EINVAL;

	free(tensor->dims);

	if (tensor->data && tensor->owned)
		free(tensor->data);

	free(tensor);
	return VACCEL_OK;
}

int vaccel_torch_tensor_set_data(struct vaccel_torch_tensor *tensor, void *data,
				 size_t size)
{
	if (!tensor)
		return VACCEL_EINVAL;

	tensor->data = data;
	tensor->size = size;

	return VACCEL_OK;
}

void *vaccel_torch_tensor_get_data(struct vaccel_torch_tensor *tensor)
{
	if (!tensor)
		return NULL;

	return tensor->data;
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
