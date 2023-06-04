/*
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

struct vaccel_session;
struct vaccel_tf_saved_model;

/* This is one-to-one mapping with tensorflow's
 * data types representation: see `tensorflow/tensorflow/c/tf_datatype.h'
 */
enum vaccel_tf_data_type {
	VACCEL_TF_FLOAT = 1,
	VACCEL_TF_DOUBLE = 2,
	VACCEL_TF_INT32 = 3,  // Int32 tensors are always in 'host' memory.
	VACCEL_TF_UINT8 = 4,
	VACCEL_TF_INT16 = 5,
	VACCEL_TF_INT8 = 6,
	VACCEL_TF_STRING = 7,
	VACCEL_TF_COMPLEX64 = 8,  // Single-precision complex
	VACCEL_TF_COMPLEX = 8,    // Old identifier kept for API backwards compatibility
	VACCEL_TF_INT64 = 9,
	VACCEL_TF_BOOL = 10,
	VACCEL_TF_QINT8 = 11,     // Quantized int8
	VACCEL_TF_QUINT8 = 12,    // Quantized uint8
	VACCEL_TF_QINT32 = 13,    // Quantized int32
	VACCEL_TF_BFLOAT16 = 14,  // Float32 truncated to 16 bits.  Only for cast ops.
	VACCEL_TF_QINT16 = 15,    // Quantized int16
	VACCEL_TF_QUINT16 = 16,   // Quantized uint16
	VACCEL_TF_UINT16 = 17,
	VACCEL_TF_COMPLEX128 = 18,  // Double-precision complex
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

struct vaccel_tf_buffer *vaccel_tf_buffer_new(void *data, size_t size);
void vaccel_tf_buffer_destroy(struct vaccel_tf_buffer *buffer);
void *vaccel_tf_buffer_take_data(struct vaccel_tf_buffer *buffer, size_t *size);
void *vaccel_tf_buffer_get_data(
	const struct vaccel_tf_buffer *buffer,
	size_t *size
);

struct vaccel_tf_node {
	/* Name of the node */
	char *name;

        /* id of the node */
        long long int id;
};

struct vaccel_tf_node *vaccel_tf_node_new(const char *name, long long int id);
void vaccel_tf_node_destroy(struct vaccel_tf_node *node);
const char *vaccel_tf_node_get_name(struct vaccel_tf_node *node);
long long int vaccel_tf_node_get_id(struct vaccel_tf_node *node);


struct vaccel_tf_tensor {
        /* Tensor's data */
        void *data;

        /* size of the data */
        size_t size;

	/* Do we own the data */
	bool owned;

        /* dimensions of the data */
        int nr_dims;
        uint32_t *dims;

        /* Data type */
        enum vaccel_tf_data_type data_type;
};

struct vaccel_tf_tensor *
vaccel_tf_tensor_new(
	int nr_dims,
	uint32_t  *dims,
	enum vaccel_tf_data_type type
);

struct vaccel_tf_tensor *
vaccel_tf_tensor_allocate(
	int nr_dims, uint32_t *dims,
	enum vaccel_tf_data_type type,
	size_t total_size
);

int vaccel_tf_tensor_destroy(struct vaccel_tf_tensor *tensor);

int vaccel_tf_tensor_set_data(
		struct vaccel_tf_tensor *tensor,
		void *data,
		size_t size
);

void *vaccel_tf_tensor_get_data(struct vaccel_tf_tensor *tensor);

struct vaccel_tf_status {
        /* TensorFlow error code */
        uint8_t error_code;

        /* TensorFlow error message */
        const char *message;
};

int vaccel_tf_session_load(
	struct vaccel_session *session,
	struct vaccel_tf_saved_model *model,
	struct vaccel_tf_status *status
);

int vaccel_tf_session_run(
	struct vaccel_session *session,
        const struct vaccel_tf_saved_model *model, const struct vaccel_tf_buffer *run_options,
        const struct vaccel_tf_node *in_nodes, struct vaccel_tf_tensor *const *in, int nr_inputs,
        const struct vaccel_tf_node *out_nodes, struct vaccel_tf_tensor **out, int nr_outputs,
        struct vaccel_tf_status *status
);


int vaccel_tf_session_delete(
	struct vaccel_session *session,
	struct vaccel_tf_saved_model *model,
	struct vaccel_tf_status *status
);
