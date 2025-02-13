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

/* One-to-one mapping with Tensorflow Lite's data types' representation.
 * See: `tensorflow/tensorflow/lite/core/c/c_api_types.h` */
enum vaccel_tflite_data_type {
	VACCEL_TFLITE_NOTYPE = 0,
	VACCEL_TFLITE_FLOAT32 = 1,
	VACCEL_TFLITE_INT32 = 2,
	VACCEL_TFLITE_UINT8 = 3,
	VACCEL_TFLITE_INT64 = 4,
	VACCEL_TFLITE_STRING = 5,
	VACCEL_TFLITE_BOOL = 6,
	VACCEL_TFLITE_INT16 = 7,
	VACCEL_TFLITE_COMPLEX64 = 8,
	VACCEL_TFLITE_INT8 = 9,
	VACCEL_TFLITE_FLOAT16 = 10,
	VACCEL_TFLITE_FLOAT64 = 11,
	VACCEL_TFLITE_COMPLEX128 = 12,
	VACCEL_TFLITE_UINT64 = 13,
	VACCEL_TFLITE_RESOURCE = 14,
	VACCEL_TFLITE_VARIANT = 15,
	VACCEL_TFLITE_UINT32 = 16,
	VACCEL_TFLITE_UINT16 = 17,
	VACCEL_TFLITE_INT4 = 18,
};

struct vaccel_tflite_tensor {
	/* tensor's data */
	void *data;

	/* size of the data */
	size_t size;

	/* do we own the data */
	bool owned;

	/* dimensions of the data */
	int nr_dims;
	int32_t *dims;

	/* data type */
	enum vaccel_tflite_data_type data_type;
};

/* Initialize TFLite tensor */
int vaccel_tflite_tensor_init(struct vaccel_tflite_tensor *tensor, int nr_dims,
			      const int32_t *dims,
			      enum vaccel_tflite_data_type type);

/* Release TFLite tensor */
int vaccel_tflite_tensor_release(struct vaccel_tflite_tensor *tensor);

/* Allocate and initialize TFLite tensor */
int vaccel_tflite_tensor_new(struct vaccel_tflite_tensor **tensor, int nr_dims,
			     const int32_t *dims,
			     enum vaccel_tflite_data_type type);

/* Allocate and initialize TFLite tensor with tensor.data of size=total_size */
int vaccel_tflite_tensor_allocate(struct vaccel_tflite_tensor **tensor,
				  int nr_dims, const int32_t *dims,
				  enum vaccel_tflite_data_type type,
				  size_t total_size);

/* Release TFLite tensor data and free TFLite tensor created with
 * vaccel_tf_tensor_new() */
int vaccel_tflite_tensor_delete(struct vaccel_tflite_tensor *tensor);

/* Set TFLite tensor tensor.data/size */
int vaccel_tflite_tensor_set_data(struct vaccel_tflite_tensor *tensor,
				  void *data, size_t size);

/* Take ownership of the TFLite tensor data */
int vaccel_tflite_tensor_take_data(struct vaccel_tflite_tensor *tensor,
				   void **data, size_t *size);

/* Load new TFLite session from model resource */
int vaccel_tflite_session_load(struct vaccel_session *session,
			       struct vaccel_resource *model);

/* Run TFLite session created with vaccel_tf_session_load() */
int vaccel_tflite_session_run(struct vaccel_session *session,
			      const struct vaccel_resource *model,
			      struct vaccel_tflite_tensor *const *in,
			      int nr_inputs, struct vaccel_tflite_tensor **out,
			      int nr_outputs, uint8_t *status);

/* Delete TFLite session created with vaccel_tf_session_load() */
int vaccel_tflite_session_delete(struct vaccel_session *session,
				 struct vaccel_resource *model);

#ifdef __cplusplus
}
#endif
