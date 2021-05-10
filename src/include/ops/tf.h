#pragma once

#include <stdint.h>
#include <stddef.h>

struct vaccel_session;
struct vaccel_tf_model;

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

struct vaccel_tf_node {
	/* Name of the node */
	char *name;

        /* id of the node */
        int64_t id;
};

struct vaccel_tf_tensor {
        /* Tensor's data */
        void *data;

        /* size of the data */
        size_t size;

        /* dimensions of the data */
        int nr_dims;
        int64_t *dims;

        /* Data type */
        enum vaccel_tf_data_type data_type;
};

struct vaccel_tf_status {
        /* TensorFlow error code */
        uint8_t error_code;

        /* TensorFlow error message */
        const char *message;
};

int vaccel_tf_model_load_graph(
	struct vaccel_session *session,
	struct vaccel_tf_model *model,
	struct vaccel_tf_status *status
);

int vaccel_tf_model_run(
	struct vaccel_session *session,
        const struct vaccel_tf_model *model, const struct vaccel_tf_buffer *run_options,
        const struct vaccel_tf_node *in_nodes, const struct vaccel_tf_tensor *in, int nr_inputs,
        const struct vaccel_tf_node *out_nodes, struct vaccel_tf_tensor *out, int nr_outputs,
        struct vaccel_tf_status *status
);
