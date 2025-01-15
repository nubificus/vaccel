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

struct vaccel_torch_tensor {
	/* tensor's data */
	void *data;

	/* size of the data */
	int64_t size;

	/* do we own the data */
	bool owned;

	/* dimensions of the data */
	int64_t nr_dims;
	int64_t *dims;

	/* data type */
	enum vaccel_torch_data_type data_type;
};

struct vaccel_torch_tensor *
vaccel_torch_tensor_new(int64_t nr_dims, const int64_t *dims,
			enum vaccel_torch_data_type type);
struct vaccel_torch_tensor *
vaccel_torch_tensor_allocate(int64_t nr_dims, int64_t *dims,
			     enum vaccel_torch_data_type type,
			     int64_t total_size);
int vaccel_torch_tensor_destroy(struct vaccel_torch_tensor *tensor);
int vaccel_torch_tensor_set_data(struct vaccel_torch_tensor *tensor, void *data,
				 int64_t size);
void *vaccel_torch_tensor_get_data(struct vaccel_torch_tensor *tensor);

struct vaccel_torch_buffer {
	/* data of the buffer */
	char *data;

	/* size of the buffer */
	size_t size;
};

struct vaccel_torch_buffer *vaccel_torch_buffer_new(char *data, size_t size);
void vaccel_torch_buffer_destroy(struct vaccel_torch_buffer *buffer);
void *vaccel_torch_buffer_take_data(struct vaccel_torch_buffer *buffer,
				    size_t *size);
void *vaccel_torch_buffer_get_data(struct vaccel_torch_buffer *buffer,
				   size_t *size);

int vaccel_torch_jitload_forward(struct vaccel_session *sess,
				 const struct vaccel_resource *model,
				 const struct vaccel_torch_buffer *run_options,
				 struct vaccel_torch_tensor **in_tensor,
				 int nr_read,
				 struct vaccel_torch_tensor **out_tensor,
				 int nr_write);

int vaccel_torch_sgemm(struct vaccel_session *sess,
		       struct vaccel_torch_tensor **in_A,
		       struct vaccel_torch_tensor **in_B,
		       struct vaccel_torch_tensor **in_C, int M, int N, int K,
		       struct vaccel_torch_tensor **out);

#ifdef __cplusplus
}
#endif
