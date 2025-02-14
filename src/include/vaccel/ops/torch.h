// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "vaccel/resource.h"
#include "vaccel/session.h"
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* One-to-one mapping with torch's fixed-with data types'
 * representation.
 * See: `pytorch/torch/csrc/api/include/torch/types.h` */
enum vaccel_torch_data_type {
	VACCEL_TORCH_BYTE = 1,
	VACCEL_TORCH_CHAR = 2,
	VACCEL_TORCH_SHORT = 3,
	VACCEL_TORCH_INT = 4,
	VACCEL_TORCH_LONG = 5,
	VACCEL_TORCH_HALF = 6,
	VACCEL_TORCH_FLOAT = 7
	// TODO: Add all data types
};

struct vaccel_torch_buffer {
	/* data of the buffer */
	char *data;

	/* size of the buffer */
	size_t size;
};

/* Initialize Torch buffer.
 * WARNING: This function will take ownership of the data. If
 * vaccel_torch_buffer_release() is called it will try to free buffer.data */
int vaccel_torch_buffer_init(struct vaccel_torch_buffer *buffer, char *data,
			     size_t size);

/* Release Torch buffer data */
int vaccel_torch_buffer_release(struct vaccel_torch_buffer *buffer);

/* Allocate and initialize Torch buffer.
 * WARNING: This function will take ownership of the data. If
 * vaccel_torch_buffer_delete() is called it will try to free buffer.data */
int vaccel_torch_buffer_new(struct vaccel_torch_buffer **buffer, char *data,
			    size_t size);

/* Release Torch buffer data and free Torch buffer created with
 * vaccel_torch_buffer_new() */
int vaccel_torch_buffer_delete(struct vaccel_torch_buffer *buffer);

/* (Re)Take ownership of the Torch buffer data.
 * Useful for releasing/deleting the Torch buffer but not the encapsulated data */
int vaccel_torch_buffer_take_data(struct vaccel_torch_buffer *buffer,
				  void **data, size_t *size);

struct vaccel_torch_tensor {
	/* tensor's data */
	void *data;

	/* size of the data */
	size_t size;

	/* do we own the data */
	bool owned;

	/* dimensions of the data */
	int64_t nr_dims;
	int64_t *dims;

	/* data type */
	enum vaccel_torch_data_type data_type;
};

/* Initialize Torch tensor */
int vaccel_torch_tensor_init(struct vaccel_torch_tensor *tensor,
			     int64_t nr_dims, const int64_t *dims,
			     enum vaccel_torch_data_type type);

/* Release Torch tensor */
int vaccel_torch_tensor_release(struct vaccel_torch_tensor *tensor);

/* Allocate and initialize Torch tensor */
int vaccel_torch_tensor_new(struct vaccel_torch_tensor **tensor,
			    int64_t nr_dims, const int64_t *dims,
			    enum vaccel_torch_data_type type);

/* Allocate and initialize Torch tensor with tensor.data of size=total_size */
int vaccel_torch_tensor_allocate(struct vaccel_torch_tensor **tensor,
				 int64_t nr_dims, const int64_t *dims,
				 enum vaccel_torch_data_type type,
				 size_t total_size);

/* Release Torch tensor data and free Torch tensor created with
 * vaccel_torch_tensor_new() */
int vaccel_torch_tensor_delete(struct vaccel_torch_tensor *tensor);

/* Set Torch tensor tensor.data/size */
int vaccel_torch_tensor_set_data(struct vaccel_torch_tensor *tensor, void *data,
				 size_t size);

/* Take ownership of the Torch tensor data */
int vaccel_torch_tensor_take_data(struct vaccel_torch_tensor *tensor,
				  void **data, size_t *size);

/* Run Torch jitload forward operation on model resource */
int vaccel_torch_jitload_forward(struct vaccel_session *sess,
				 const struct vaccel_resource *model,
				 const struct vaccel_torch_buffer *run_options,
				 struct vaccel_torch_tensor **in_tensor,
				 int nr_read,
				 struct vaccel_torch_tensor **out_tensor,
				 int nr_write);

/* Run Torch sgemm operation */
int vaccel_torch_sgemm(struct vaccel_session *sess,
		       struct vaccel_torch_tensor **in_A,
		       struct vaccel_torch_tensor **in_B,
		       struct vaccel_torch_tensor **in_C, int M, int N, int K,
		       struct vaccel_torch_tensor **out);

#ifdef __cplusplus
}
#endif
