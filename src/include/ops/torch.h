#ifndef __VACCEL_TORCH_H__
#define __VACCEL_TORCH_H__

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

struct vaccel_session;
struct vaccel_torch_saved_model;

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

struct vaccel_torch_tensor *
vaccel_torch_tensor_new(
	int nr_dims,
	int64_t *dims,
	enum vaccel_torch_data_type type
);

struct vaccel_torch_tensor *
vaccel_torch_tensor_allocate(
	int nr_dims, int64_t *dims,
	enum vaccel_torch_data_type type,
	size_t total_size
);

struct vaccel_torch_buffer {
	/* data of the buffer */
	char *data;

	/* size of the buffer */
	size_t size;
};

void vaccel_torch_buffer_destroy(struct vaccel_torch_buffer *buffer);
void *vaccel_torch_buffer_take_data(struct vaccel_torch_buffer *buffer, size_t *size);
void *vaccel_torch_buffer_get_data(
		struct vaccel_torch_buffer *buffer,
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
		const struct vaccel_torch_saved_model *model, 
		const struct vaccel_torch_buffer *run_options,
		struct vaccel_torch_tensor **in_tensor,
		int nr_read,
		struct vaccel_torch_tensor **out_tensor,
		int nr_write);

int vaccel_torch_sgemm(struct vaccel_session *sess,
		struct vaccel_torch_tensor **in_A,
		struct vaccel_torch_tensor **in_B,
		struct vaccel_torch_tensor **in_C,
		int M, int N, int K,
		struct vaccel_torch_tensor **out);

int vaccel_torch_tensor_set_data(
    struct vaccel_torch_tensor *tensor,
    void *data,
    size_t size
);

void *vaccel_torch_tensor_get_data(struct vaccel_torch_tensor *tensor);
struct vaccel_torch_buffer *vaccel_torch_buffer_new(char *data, size_t size);
#ifdef __cplusplus
}
#endif

#endif /* __VACCEL_TORCH_H__ */
