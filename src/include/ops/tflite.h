// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "session.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct vaccel_single_model;

/* This is one-to-one mapping with tensorflow lite's
 * data types representation: see 'tensorflow/tensorflow/lite/core/c/c_api_types.h'
 */
enum vaccel_tflite_type {
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
	/* Tensor's data */
	void *data;

	/* Size of the data */
	size_t size;

	/* Do we own the data */
	bool owned;

	/* Dimensions of the data */
	int nr_dims;
	int32_t *dims;

	/* Data type */
	enum vaccel_tflite_type data_type;
};

struct vaccel_tflite_tensor *
vaccel_tflite_tensor_new(int nr_dims, int32_t *dims,
			 enum vaccel_tflite_type type);

struct vaccel_tflite_tensor *
vaccel_tflite_tensor_allocate(int nr_dims, int32_t *dims,
			      enum vaccel_tflite_type type, size_t total_size);

int vaccel_tflite_tensor_destroy(struct vaccel_tflite_tensor *tensor);

int vaccel_tflite_tensor_set_data(struct vaccel_tflite_tensor *tensor,
				  void *data, size_t size);

void *vaccel_tflite_tensor_get_data(struct vaccel_tflite_tensor *tensor);

int vaccel_tflite_session_load(struct vaccel_session *session,
			       struct vaccel_single_model *model);

int vaccel_tflite_session_run(struct vaccel_session *session,
			      const struct vaccel_single_model *model,
			      struct vaccel_tflite_tensor *const *in,
			      int nr_inputs, struct vaccel_tflite_tensor **out,
			      int nr_outputs, uint8_t *status);

int vaccel_tflite_session_delete(struct vaccel_session *session,
				 struct vaccel_single_model *model);
