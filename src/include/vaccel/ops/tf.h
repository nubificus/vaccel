// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "vaccel/resource.h"
#include "vaccel/session.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* One-to-one mapping with tensorflow's data types' representation.
 * See: `tensorflow/tensorflow/c/tf_datatype.h` */
enum vaccel_tf_data_type {
	VACCEL_TF_FLOAT = 1,
	VACCEL_TF_DOUBLE = 2,
	VACCEL_TF_INT32 = 3, // Int32 tensors are always in 'host' memory.
	VACCEL_TF_UINT8 = 4,
	VACCEL_TF_INT16 = 5,
	VACCEL_TF_INT8 = 6,
	VACCEL_TF_STRING = 7,
	VACCEL_TF_COMPLEX64 = 8, // Single-precision complex
	VACCEL_TF_COMPLEX = 8, // Old identifier for backwards compatibility
	VACCEL_TF_INT64 = 9,
	VACCEL_TF_BOOL = 10,
	VACCEL_TF_QINT8 = 11, // Quantized int8
	VACCEL_TF_QUINT8 = 12, // Quantized uint8
	VACCEL_TF_QINT32 = 13, // Quantized int32
	VACCEL_TF_BFLOAT16 =
		14, // Float32 truncated to 16 bits. Only for cast ops
	VACCEL_TF_QINT16 = 15, // Quantized int16
	VACCEL_TF_QUINT16 = 16, // Quantized uint16
	VACCEL_TF_UINT16 = 17,
	VACCEL_TF_COMPLEX128 = 18, // Double-precision complex
	VACCEL_TF_HALF = 19,
	VACCEL_TF_RESOURCE = 20,
	VACCEL_TF_VARIANT = 21,
	VACCEL_TF_UINT32 = 22,
	VACCEL_TF_UINT64 = 23,
};

struct vaccel_tf_buffer {
	/* data of the buffer */
	void *data;

	/* size of the buffer */
	size_t size;
};

/* Initialize TF buffer.
 * WARNING: This function will take ownership of the data. If
 * vaccel_tf_buffer_release() is called it will try to free buffer.data */
int vaccel_tf_buffer_init(struct vaccel_tf_buffer *buffer, void *data,
			  size_t size);

/* Release TF buffer data */
int vaccel_tf_buffer_release(struct vaccel_tf_buffer *buffer);

/* Allocate and initialize TF buffer.
 * WARNING: This function will take ownership of the data. If
 * vaccel_tf_buffer_delete() is called it will try to free buffer.data */
int vaccel_tf_buffer_new(struct vaccel_tf_buffer **buffer, void *data,
			 size_t size);

/* Release TF buffer data and free TF buffer created with
 * vaccel_tf_buffer_new() */
int vaccel_tf_buffer_delete(struct vaccel_tf_buffer *buffer);

/* (Re)Take ownership of the TF buffer data.
 * Useful for releasing/deleting the TF buffer but not the encapsulated data */
int vaccel_tf_buffer_take_data(struct vaccel_tf_buffer *buffer, void **data,
			       size_t *size);

struct vaccel_tf_node {
	/* name of the node */
	char *name;

	/* id of the node */
	int id;
};

/* Initialize TF node */
int vaccel_tf_node_init(struct vaccel_tf_node *node, const char *name, int id);

/* Release TF node data */
int vaccel_tf_node_release(struct vaccel_tf_node *node);

/* Allocate and initialize TF node */
int vaccel_tf_node_new(struct vaccel_tf_node **node, const char *name, int id);

/* Release TF node data and free TF node created with vaccel_tf_node_new() */
int vaccel_tf_node_delete(struct vaccel_tf_node *node);

struct vaccel_tf_tensor {
	/* tensor's data */
	void *data;

	/* size of the data */
	size_t size;

	/* do we own the data */
	bool owned;

	/* dimensions of the data */
	int nr_dims;
	int64_t *dims;

	/* data type */
	enum vaccel_tf_data_type data_type;
};

/* Initialize TF tensor */
int vaccel_tf_tensor_init(struct vaccel_tf_tensor *tensor, int nr_dims,
			  const int64_t *dims, enum vaccel_tf_data_type type);

/* Release TF tensor */
int vaccel_tf_tensor_release(struct vaccel_tf_tensor *tensor);

/* Allocate and initialize TF tensor */
int vaccel_tf_tensor_new(struct vaccel_tf_tensor **tensor, int nr_dims,
			 const int64_t *dims, enum vaccel_tf_data_type type);

/* Allocate and initialize TF tensor with tensor.data of size=total_size */
int vaccel_tf_tensor_allocate(struct vaccel_tf_tensor **tensor, int nr_dims,
			      const int64_t *dims,
			      enum vaccel_tf_data_type type, size_t total_size);

/* Release TF tensor data and free TF tensor created with
 * vaccel_tf_tensor_new() */
int vaccel_tf_tensor_delete(struct vaccel_tf_tensor *tensor);

/* Set TF tensor tensor.data/size */
int vaccel_tf_tensor_set_data(struct vaccel_tf_tensor *tensor, void *data,
			      size_t size);

/* Take ownership of the TF tensor data */
int vaccel_tf_tensor_take_data(struct vaccel_tf_tensor *tensor, void **data,
			       size_t *size);

struct vaccel_tf_status {
	/* tensorFlow error code */
	uint8_t error_code;

	/* tensorFlow error message */
	char *message;
};

/* Initialize TF status */
int vaccel_tf_status_init(struct vaccel_tf_status *status, uint8_t error_code,
			  const char *message);

/* Release TF status */
int vaccel_tf_status_release(struct vaccel_tf_status *status);

/* Allocate and initialize TF status */
int vaccel_tf_status_new(struct vaccel_tf_status **status, uint8_t error_code,
			 const char *message);

/* Release TF status data and free TF status created with
 * vaccel_tf_status_new() */
int vaccel_tf_status_delete(struct vaccel_tf_status *status);

/* Load new TF session from model resource */
int vaccel_tf_session_load(struct vaccel_session *sess,
			   struct vaccel_resource *model,
			   struct vaccel_tf_status *status);

/* Run TF session created with vaccel_tf_session_load() */
int vaccel_tf_session_run(struct vaccel_session *sess,
			  const struct vaccel_resource *model,
			  const struct vaccel_tf_buffer *run_options,
			  const struct vaccel_tf_node *in_nodes,
			  struct vaccel_tf_tensor *const *in, int nr_inputs,
			  const struct vaccel_tf_node *out_nodes,
			  struct vaccel_tf_tensor **out, int nr_outputs,
			  struct vaccel_tf_status *status);

/* Delete TF session created with vaccel_tf_session_load() */
int vaccel_tf_session_delete(struct vaccel_session *sess,
			     struct vaccel_resource *model,
			     struct vaccel_tf_status *status);

#ifdef __cplusplus
}
#endif
