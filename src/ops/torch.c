#include "torch.h"
#include "error.h"
#include "ops/genop.h"
#include "ops/torch.h"
#include "plugin.h"
#include "log.h"
#include "vaccel_ops.h"
#include "genop.h"

#include "session.h"
#include <pthread.h>
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

// Destory Torch buffer data
void vaccel_torch_buffer_destroy(struct vaccel_torch_buffer *buffer)
{
	if (buffer->data)
		free(buffer->data);
	free(buffer);
}

void *vaccel_torch_buffer_take_data(struct vaccel_torch_buffer *buffer, size_t *size)
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
void *vaccel_torch_buffer_get_data(struct vaccel_torch_buffer *buffer, size_t *size)
{
	if (!buffer)
		return NULL;
	
	*size = buffer->size;
	return buffer->data;
}

struct vaccel_torch_tensor *
vaccel_torch_tensor_new(int nr_dims, int64_t *dims, enum vaccel_torch_data_type type)
{
    struct vaccel_torch_tensor *ret;

    ret = calloc(1, sizeof(*ret));
    if (!ret)
        return NULL;

    ret->data_type = type;
    ret->nr_dims = nr_dims;
	// void *calloc(size_t numElements, size_t sizeOfElement);
    ret->dims = calloc(nr_dims, sizeof(*dims));
    if (!ret->dims) {
        free(ret);
        return NULL;
    }

    if (dims)
        memcpy(ret->dims, dims, nr_dims * sizeof(*dims));

    return ret;
}

struct vaccel_torch_tensor *
vaccel_torch_tensor_allocate(
    int nr_dims, int64_t *dims,
    enum vaccel_torch_data_type type,
    size_t total_size
) {
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

int vaccel_torch_tensor_set_data(
    struct vaccel_torch_tensor *tensor,
    void *data,
    size_t size
) {
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


#if 0
// The unpack solution
int vaccel_torch_unpack(struct vaccel_session *sess, struct vaccel_arg *read,
		int nr_read, struct vaccel_arg *write, int nr_write)
{
	if (nr_read != 3) {
		vaccel_error("Wrong number of read arguments in torch image loading: %d",
					nr_read);
		
		return VACCEL_EINVAL;
	}

	if (nr_write != 1) {
		vaccel_error("Wrong number of write arguments in torch image loading: %d",
					nr_write);
		
		return VACCEL_EINVAL;
	}
	
	
	char* model_path = (char*)read[0].buf;
	char * img = (char*)read[1].buf;
	size_t img_size = *(size_t*)read[2].buf;
	char *tags = (char*)write[0].buf;
	
    return vaccel_torch_jitload_forward(sess, model_path, img, img_size, &tags);
}
#endif

#if 0
int vaccel_torch_jitload_forward(struct vaccel_session *sess,
		const char* model_path, 
		const char* img, 
		size_t img_size, 
		char **tags)
{
	if (!sess)
		return VACCEL_EINVAL;
	vaccel_debug("session:%u Looking for plugin implementing torch_jitload_forward operation",
						sess->session_id);
	int (*plugin_op)() = get_plugin_op(VACCEL_TORCH_JITLOAD_FORWARD);
	if (!plugin_op)
		return VACCEL_ENOTSUP;
	return plugin_op(sess, model_path, img, img_size, tags);
}
#endif

// struct vaccel_arg *write -> char **tags
int vaccel_torch_jitload_forward(struct vaccel_session *sess,
		const struct vaccel_torch_saved_model *model,
		const struct vaccel_torch_buffer *run_options,
		struct vaccel_torch_tensor **in_tensor,
		int nr_read, 
		struct vaccel_torch_tensor **out_tensor,
		int nr_write)
{
	if (!sess)
		return VACCEL_EINVAL;
	
	vaccel_debug("session:%u Looking for plugin implementing torch_jitload_forward operation",
						sess->session_id);
	
	// struct vaccel_arg * -> char **
	int (*plugin_op)(
			struct vaccel_session *,
			const struct vaccel_torch_saved_model *, 
		    const struct vaccel_torch_buffer *,
			struct vaccel_torch_tensor **,
			int,
			struct vaccel_torch_tensor **,
			int
			) = get_plugin_op(VACCEL_TORCH_JITLOAD_FORWARD, sess->hint);

	if (!plugin_op)
		return VACCEL_ENOTSUP;
	// write -> tags
	return plugin_op(sess, model, run_options, in_tensor, nr_read, out_tensor, nr_write);
}

int vaccel_torch_sgemm(struct vaccel_session *sess,
	struct vaccel_torch_tensor **in_A,
	struct vaccel_torch_tensor **in_B,
	struct vaccel_torch_tensor **in_C,
	int M, int N, int K,
	struct vaccel_torch_tensor **out)
{
	if (!sess)
		return VACCEL_EINVAL;

	vaccel_debug("session:%u Looking for plugin implementing torch sgemm operation",
						sess->session_id);
	int (*plugin_op)(
			struct vaccel_session *,
			struct vaccel_torch_tensor **,
			struct vaccel_torch_tensor **,
			struct vaccel_torch_tensor **,
			int ,
			int ,
			int ,
			struct vaccel_torch_tensor **
			) = get_plugin_op(VACCEL_TORCH_SGEMM, sess->hint);
	if (!plugin_op) {
		vaccel_debug("Plugin loading failed");
		return VACCEL_ENOTSUP;
	}
	return plugin_op(sess, in_A, in_B, in_C, M, N, K, out);
}
