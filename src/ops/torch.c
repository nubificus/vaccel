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
vaccel_torch_tensor_new(int nr_dims, const int64_t *dims,
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
		for (int i = 0; i < nr_dims; i++) {
			ret->dims[i] = (int32_t)dims[i];
		}
	}

	return ret;
}

struct vaccel_torch_tensor *
vaccel_torch_tensor_allocate(int nr_dims, int64_t *dims,
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

	// why do we free here?
	if (tensor->data && tensor->owned)
		free(tensor->data);

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
		plugin_get_op_func(VACCEL_TORCH_JITLOAD_FORWARD, sess->hint);
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
		plugin_get_op_func(VACCEL_TORCH_SGEMM, sess->hint);
	if (!plugin_torch_sgemm) {
		vaccel_debug("Plugin loading failed");
		return VACCEL_ENOTSUP;
	}

	return plugin_torch_sgemm(sess, in_A, in_B, in_C, M, N, K, out);
}

/* ===== The new functions ===== */

static size_t data_types_length[] = {1, 1, 2, 4, 4, 2, 4};

static size_t calc_nr_bytes(int nr_dims, int *dims,
			    enum vaccel_data_type type)
{
	size_t elem_size = data_types_length[type];
	size_t total_elems = 1;

	for (int i = 0; i < nr_dims; ++i)
		total_elems *= dims[i];

	return total_elems * elem_size;
}

typedef int (*tensor_init_from_buf_fn_t)(
	struct vaccel_session *, vaccel_tensor_t *, int,
	int *, void *, enum vaccel_data_type);

int vaccel_tensor_init_from_buf(struct vaccel_session *sess,
				vaccel_tensor_t *tensor,
				int nr_dims, int *dims, void *buf,
				enum vaccel_data_type type)
{
	if (!sess || !tensor || !dims || !buf)
		return VACCEL_EINVAL;

	vaccel_debug(
		"session:%" PRId64
		" Looking for plugin implementing `vaccel_tensor_init_from_buf` operation",
		sess->id);

	tensor_init_from_buf_fn_t plugin_tensor_init_from_buf =
		plugin_get_op_func(VACCEL_TENSOR_INIT_FROM_BUF, sess->hint);

	if (!plugin_tensor_init_from_buf)
		return VACCEL_ENOTSUP;

	return plugin_tensor_init_from_buf(sess, tensor, nr_dims,
					   dims, buf, type);
}

typedef int (*tensor_init_from_res_fn_t)(
	struct vaccel_session *, struct vaccel_resource *,
	vaccel_tensor_t *, int, int *, enum vaccel_data_type);

int vaccel_tensor_init_from_res(struct vaccel_session *sess,
				struct vaccel_resource *res,
				vaccel_tensor_t *tensor,
				int nr_dims, int *dims,
				enum vaccel_data_type type)
{
	if (!sess || !res || !tensor || !dims)
		return VACCEL_EINVAL;

	if (res->type != VACCEL_RESOURCE_TENSOR) {
		vaccel_error("Only resources of type `tensor` can be used"
			     " for this operation");
		return VACCEL_EPERM;
	}

	if (!vaccel_session_has_resource(sess, res)) {
		vaccel_error("Resource %" PRId64
			     " is not registered to session %" PRId64 "",
			     res->id, sess->id);
		return VACCEL_EPERM;
	}

	vaccel_debug(
		"session:%" PRId64
		" Looking for plugin implementing `vaccel_tensor_init_from_res` operation",
		sess->id);

	tensor_init_from_res_fn_t plugin_tensor_init_from_res =
		plugin_get_op_func(VACCEL_TENSOR_INIT_FROM_RES, sess->hint);

	if (!plugin_tensor_init_from_res)
		return VACCEL_ENOTSUP;

	return plugin_tensor_init_from_res(sess, res, tensor, nr_dims,
					   dims, type);
}

int vaccel_tensor_init(struct vaccel_session *sess,
		       vaccel_tensor_t *tensor,
		       int nr_dims, int *dims, void *data,
		       enum vaccel_data_type type)
{
	int is_rpc_plugin = 0;

	if (!is_rpc_plugin) {
		return vaccel_tensor_init_from_buf(sess, tensor, nr_dims,
						   dims, data, type);
	}

	int ret;
	struct vaccel_resource res;
	int nr_bytes = calc_nr_bytes(nr_dims, dims, type);

	ret = vaccel_resource_init_from_buf(&res, data, nr_bytes,
					    VACCEL_RESOURCE_TENSOR, NULL);
	if (ret != VACCEL_OK)
		return ret;

	ret = vaccel_resource_register(&res, sess);
	if (ret != VACCEL_OK)
		return ret;

	ret = vaccel_tensor_init_from_res(sess, &res, tensor,
					  nr_dims, dims, type);
	if (ret != VACCEL_OK)
		return ret;

	ret = vaccel_resource_unregister(&res, sess);
	if (ret != VACCEL_OK)
		return ret;

	ret = vaccel_resource_release(&res);
	if (ret != VACCEL_OK)
		return ret;

	return VACCEL_OK;
}

typedef int (*tensor_alloc_fn_t)(
	struct vaccel_session *, vaccel_tensor_t *,
	int, int *, enum vaccel_data_type);


int vaccel_tensor_alloc(struct vaccel_session *sess,
			vaccel_tensor_t *tensor,
			int nr_dims, int *dims,
			enum vaccel_data_type type)
{
	if (!sess || !tensor || !dims)
		return VACCEL_EINVAL;

	vaccel_debug(
		"session:%" PRId64
		" Looking for plugin implementing `vaccel_tensor_alloc` operation",
		sess->id);

	tensor_alloc_fn_t plugin_tensor_alloc =
		plugin_get_op_func(VACCEL_TENSOR_ALLOC, sess->hint);

	if (!plugin_tensor_alloc)
		return VACCEL_ENOTSUP;

	return plugin_tensor_alloc(sess, tensor, nr_dims, dims, type);
}

typedef int (*tensor_forward_fn_t)(
	struct vaccel_session *, struct vaccel_resource *,
	int, vaccel_tensor_t *, vaccel_tensor_t *);

int vaccel_tensor_forward(struct vaccel_session *sess,
			  struct vaccel_resource *model_res,
			  int nr_in, vaccel_tensor_t *in,
			  vaccel_tensor_t *out)
{
	if (!sess || !model_res || !in || !out || nr_in <= 0)
		return VACCEL_EINVAL;

	vaccel_debug(
		"session:%" PRId64
		" Looking for plugin implementing `tensor_forward` operation",
		sess->id);

	if (model_res->type != VACCEL_RESOURCE_MODEL) {
		vaccel_error(
			"Invalid resource type: expected VACCEL_RESOURCE_MODEL");
		return VACCEL_EINVAL;
	}

	if (!vaccel_session_has_resource(sess, model_res)) {
		vaccel_error("Resource %" PRId64
			     " is not registered to session %" PRId64 "",
			     model_res->id, sess->id);
		return VACCEL_EPERM;
	}

	tensor_forward_fn_t plugin_tensor_forward =
		plugin_get_op_func(VACCEL_TENSOR_FORWARD, sess->hint);
	if (!plugin_tensor_forward)
		return VACCEL_ENOTSUP;

	return plugin_tensor_forward(sess, model_res, nr_in, in, out);
}

typedef int (*tensor_get_data_fn_t)(
	struct vaccel_session *, vaccel_tensor_t *, void *);

int vaccel_tensor_get_data(struct vaccel_session *sess,
			   vaccel_tensor_t *tensor,
			   void *buf)
{
	if (!sess || !tensor || !buf)
		return VACCEL_EINVAL;

	vaccel_debug(
		"session:%" PRId64
		" Looking for plugin implementing `vaccel_tensor_get_data` operation",
		sess->id);

	tensor_get_data_fn_t plugin_tensor_get_data =
		plugin_get_op_func(VACCEL_TENSOR_GET_DATA, sess->hint);

	if (!plugin_tensor_get_data)
		return VACCEL_ENOTSUP;

	return plugin_tensor_get_data(sess, tensor, buf);
}

typedef int (*tensor_permute_fn_t)(
	struct vaccel_session *, vaccel_tensor_t *,
	int, int *);

int vaccel_tensor_permute(struct vaccel_session *sess,
			  vaccel_tensor_t *tensor,
			  int nr_dims, int *new_dims)
{
	if (!sess || !tensor || !new_dims || nr_dims <= 0)
		return VACCEL_EINVAL;

	vaccel_debug(
		"session:%" PRId64
		" Looking for plugin implementing `vaccel_tensor_permute` operation",
		sess->id);

	tensor_permute_fn_t plugin_tensor_permute =
		plugin_get_op_func(VACCEL_TENSOR_PERMUTE, sess->hint);

	if (!plugin_tensor_permute)
		return VACCEL_ENOTSUP;

	return plugin_tensor_permute(sess, tensor, nr_dims,
				     new_dims);
}

typedef int (*tensor_get_sub_fn_t)(
	struct vaccel_session *, vaccel_tensor_t *,
	vaccel_tensor_t*, int, int *);

int vaccel_tensor_get_sub(struct vaccel_session *sess,
			  vaccel_tensor_t *orig,
			  vaccel_tensor_t *sub,
			  int nr_sub_dims, int *sub_dims)
{
	if (!sess || !orig || !sub || !sub_dims || nr_sub_dims <= 0)
		return VACCEL_EINVAL;

	vaccel_debug(
		"session:%" PRId64
		" Looking for plugin implementing `vaccel_tensor_get_sub` operation",
		sess->id);

	tensor_get_sub_fn_t plugin_tensor_get_sub =
		plugin_get_op_func(VACCEL_TENSOR_GET_SUB, sess->hint);

	if (!plugin_tensor_get_sub)
		return VACCEL_ENOTSUP;

	return plugin_tensor_get_sub(sess, orig, sub,
				     nr_sub_dims, sub_dims);
}

typedef int (*tensor_sub_val_fn_t)(
	struct vaccel_session *, vaccel_tensor_t *,
	void *, enum vaccel_data_type);

int vaccel_tensor_sub_val(struct vaccel_session *sess,
			  vaccel_tensor_t* tensor, void *val,
			  enum vaccel_data_type type)
{
	if (!sess || !tensor || !val)
		return VACCEL_EINVAL;

	vaccel_debug(
		"session:%" PRId64
		" Looking for plugin implementing `vaccel_tensor_sub_val` operation",
		sess->id);

	tensor_sub_val_fn_t plugin_tensor_sub_val =
		plugin_get_op_func(VACCEL_TENSOR_SUB_VAL, sess->hint);

	if (!plugin_tensor_sub_val)
		return VACCEL_ENOTSUP;

	/* Max value is 4 bytes, so we
	 * allocate a 4 bytes buffer for
	 * all types. */
	unsigned char val_buf[4] = { 0 };

	size_t type_len = data_types_length[type];
	memcpy(val_buf, val, type_len);

	return plugin_tensor_sub_val(sess, tensor, val_buf, type);
}

typedef int (*tensor_div_val_fn_t)(
	struct vaccel_session *, vaccel_tensor_t *,
	void *, enum vaccel_data_type);

int vaccel_tensor_div_val(struct vaccel_session *sess,
			  vaccel_tensor_t* tensor, void *val,
			  enum vaccel_data_type type)
{
	if (!sess || !tensor || !val)
		return VACCEL_EINVAL;

	vaccel_debug(
		"session:%" PRId64
		" Looking for plugin implementing `vaccel_tensor_div_val` operation",
		sess->id);

	tensor_div_val_fn_t plugin_tensor_div_val =
		plugin_get_op_func(VACCEL_TENSOR_DIV_VAL, sess->hint);

	if (!plugin_tensor_div_val)
		return VACCEL_ENOTSUP;

	/* Max value is 4 bytes, so we
	 * allocate a 4 bytes buffer for
	 * all types. */
	unsigned char val_buf[4] = { 0 };

	size_t type_len = data_types_length[type];
	memcpy(val_buf, val, type_len);

	return plugin_tensor_div_val(sess, tensor, val_buf, type);
}

typedef int (*tensor_add_val_fn_t)(
	struct vaccel_session *, vaccel_tensor_t *,
	void *, enum vaccel_data_type);

int vaccel_tensor_add_val(struct vaccel_session *sess,
			  vaccel_tensor_t* tensor, void *val,
			  enum vaccel_data_type type)
{
	if (!sess || !tensor || !val)
		return VACCEL_EINVAL;

	vaccel_debug(
		"session:%" PRId64
		" Looking for plugin implementing `vaccel_tensor_add_val` operation",
		sess->id);

	tensor_add_val_fn_t plugin_tensor_add_val =
		plugin_get_op_func(VACCEL_TENSOR_ADD_VAL, sess->hint);

	if (!plugin_tensor_add_val)
		return VACCEL_ENOTSUP;

	/* Max value is 4 bytes, so we
	 * allocate a 4 bytes buffer for
	 * all types. */
	unsigned char val_buf[4] = { 0 };

	size_t type_len = data_types_length[type];
	memcpy(val_buf, val, type_len);

	return plugin_tensor_add_val(sess, tensor, val_buf, type);
}

typedef int (*tensor_mul_val_fn_t)(
	struct vaccel_session *, vaccel_tensor_t *,
	void *, enum vaccel_data_type);

int vaccel_tensor_mul_val(struct vaccel_session *sess,
			  vaccel_tensor_t* tensor, void *val,
			  enum vaccel_data_type type)
{
	if (!sess || !tensor || !val)
		return VACCEL_EINVAL;

	vaccel_debug(
		"session:%" PRId64
		" Looking for plugin implementing `vaccel_tensor_mul_val` operation",
		sess->id);

	tensor_mul_val_fn_t plugin_tensor_mul_val =
		plugin_get_op_func(VACCEL_TENSOR_MUL_VAL, sess->hint);

	if (!plugin_tensor_mul_val)
		return VACCEL_ENOTSUP;

	/* Max value is 4 bytes, so we
	 * allocate a 4 bytes buffer for
	 * all types. */
	unsigned char val_buf[4] = { 0 };

	size_t type_len = data_types_length[type];
	memcpy(val_buf, val, type_len);

	return plugin_tensor_mul_val(sess, tensor, val_buf, type);
}
