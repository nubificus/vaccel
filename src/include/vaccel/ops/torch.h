// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "vaccel/session.h"
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct vaccel_session;
struct vaccel_resource;

/*

// Fixed width dtypes.
// ./pytorch/torch/csrc/api/include/torch/types.h
constexpr auto kUInt8 = at::kByte;
constexpr auto kInt8 = at::kChar;
constexpr auto kInt16 = at::kShort;
constexpr auto kInt32 = at::kInt;
constexpr auto kInt64 = at::kLong;
constexpr auto kFloat16 = at::kHalf;
constexpr auto kFloat32 = at::kFloat;

// ./pytorch/torch/csrc/jit/codegen/cuda/type.h 
enum class DataType {
  Double,
  Float,
  Half,
  Int,
  Index,
  Int32,
  Bool,
  BFloat16,
  ComplexFloat,
  ComplexDouble,
  // Vectorized types, used for reinterpret casting views
  // TODO: add more vectorized types
  Double_2,
  Float_2,
  // Null
  Null
};

 */

enum vaccel_torch_data_type {
	VACCEL_TORCH_BYTE = 1,
	VACCEL_TORCH_CHAR = 2,
	VACCEL_TORCH_SHORT = 3,
	VACCEL_TORCH_INT = 4,
	VACCEL_TORCH_LONG = 5,
	VACCEL_TORCH_HALF = 6,
	VACCEL_TORCH_FLOAT = 7
};

struct vaccel_torch_tensor {
	void *data;
	size_t size;
	uint8_t owned;

	int nr_dims;
	int32_t *dims;

	/* Data type */
	enum vaccel_torch_data_type data_type;
};

typedef struct {
	void *framework_tensor_ptr;
} vaccel_tensor_t;

enum vaccel_data_type {
	VACCEL_BYTE = 0,
	VACCEL_CHAR = 1,
	VACCEL_SHORT = 2,
	VACCEL_INT = 3,
	VACCEL_LONG = 4,
	VACCEL_HALF = 5,
	VACCEL_FLOAT = 6
};


/* Operations */
int vaccel_tensor_init_from_buf(struct vaccel_session *sess,
				vaccel_tensor_t *tensor,
				int nr_dims, int *dims, void *buf,
				enum vaccel_data_type type);

int vaccel_tensor_init_from_res(struct vaccel_session *sess,
				struct vaccel_resource *res,
				vaccel_tensor_t *tensor,
				int nr_dims, int *dims,
				enum vaccel_data_type type);

/* Allocate the tensor and write the content from `data`.
 * Internally, use resources to transfer the data in case
 * of rpc plugin.
 * Also, we infer the data length from dims and data type.
 */
int vaccel_tensor_init(struct vaccel_session *sess,
		       vaccel_tensor_t *tensor,
		       int nr_dims, int *dims, void *data,
		       enum vaccel_data_type type);

/* Memory allocation only, no specific data - use for output tensors */
int vaccel_tensor_alloc(struct vaccel_session *sess,
			vaccel_tensor_t *tensor,
			int nr_dims, int *dims,
			enum vaccel_data_type type);

/* Write the content of the tensor to a buffer.
 * No need to know the number of bytes, since
 * we can infer it from dimensions and data type.
 * We may also add a query call for this.
 */
int vaccel_tensor_get_data(struct vaccel_session *sess,
			   vaccel_tensor_t *tensor,
			   void *buf);

/* Permute tensor dimensions - Run directly to the torch::tensor */
int vaccel_tensor_permute(struct vaccel_session *sess,
			  vaccel_tensor_t *tensor,
			  int nr_dims, int *new_dims);

/* Delete the live torch::tensor */
int vaccel_tensor_destroy(struct vaccel_session *sess,
			  vaccel_tensor_t *tensor);

/* Forward */
int vaccel_tensor_forward(struct vaccel_session *sess,
			  struct vaccel_resource *model_res,
			  int nr_in, vaccel_tensor_t *in,
			  vaccel_tensor_t *out);

int vaccel_tensor_get_sub(struct vaccel_session *sess,
			  vaccel_tensor_t *orig,
			  vaccel_tensor_t *sub,
			  int nr_sub_dims, int *sub_dims);

int vaccel_tensor_sub_val(struct vaccel_session *sess,
			  vaccel_tensor_t* tensor, void *val,
			  enum vaccel_data_type type);

int vaccel_tensor_div_val(struct vaccel_session *sess,
			  vaccel_tensor_t* tensor, void *val,
			  enum vaccel_data_type type);

int vaccel_tensor_add_val(struct vaccel_session *sess,
			  vaccel_tensor_t* tensor, void *val,
			  enum vaccel_data_type type);

int vaccel_tensor_mul_val(struct vaccel_session *sess,
			  vaccel_tensor_t* tensor, void *val,
			  enum vaccel_data_type type);

/* ===== Old API ===== */
struct vaccel_torch_tensor *
vaccel_torch_tensor_new(int nr_dims, const int64_t *dims,
			enum vaccel_torch_data_type type);

struct vaccel_torch_tensor *
vaccel_torch_tensor_allocate(int nr_dims, int64_t *dims,
			     enum vaccel_torch_data_type type,
			     size_t total_size);

struct vaccel_torch_buffer {
	/* data of the buffer */
	char *data;

	/* size of the buffer */
	size_t size;
};

void vaccel_torch_buffer_destroy(struct vaccel_torch_buffer *buffer);
void *vaccel_torch_buffer_take_data(struct vaccel_torch_buffer *buffer,
				    size_t *size);
void *vaccel_torch_buffer_get_data(struct vaccel_torch_buffer *buffer,
				   size_t *size);

int vaccel_torch_tensor_destroy(struct vaccel_torch_tensor *tensor);

#if 0
// The unpack solution
int vaccel_torch_unpack(struct vaccel_session *sess, 
		struct vaccel_arg *read,
		int nr_read, 
		struct vaccel_arg *write, 
		int nr_write);

// Change: const char* model_path -> struct vaccel_torch_saved_model *model
// const char* buffer -> const struct vaccel_torch_buffer* buffer

int vaccel_torch_jitload_forward(struct vaccel_session *sess,
		const struct vaccel_torch_saved_model *model, 
		const char* buffer,
		size_t bufferLength, 
		char**tags);
#endif

// struct vaccel_arg *write -> char **tags
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

int vaccel_torch_tensor_set_data(struct vaccel_torch_tensor *tensor, void *data,
				 size_t size);

void *vaccel_torch_tensor_get_data(struct vaccel_torch_tensor *tensor);
struct vaccel_torch_buffer *vaccel_torch_buffer_new(char *data, size_t size);
#ifdef __cplusplus
}
#endif
