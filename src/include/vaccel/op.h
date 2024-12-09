// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum vaccel_op_type {
	VACCEL_NO_OP = 0,
	VACCEL_BLAS_SGEMM, /* 1 */
	VACCEL_IMG_CLASS, /* 2 */
	VACCEL_IMG_DETEC, /* 3 */
	VACCEL_IMG_SEGME, /* 4 */
	VACCEL_IMG_POSE, /* 5 */
	VACCEL_IMG_DEPTH, /* 6 */
	VACCEL_EXEC, /* 7 */
	VACCEL_TF_MODEL_NEW, /* 8 */
	VACCEL_TF_MODEL_DESTROY, /* 9 */
	VACCEL_TF_MODEL_REGISTER, /* 10 */
	VACCEL_TF_MODEL_UNREGISTER, /* 11 */
	VACCEL_TF_SESSION_LOAD, /* 12 */
	VACCEL_TF_SESSION_RUN, /* 13 */
	VACCEL_TF_SESSION_DELETE, /* 14 */
	VACCEL_MINMAX, /* 15 */
	VACCEL_F_ARRAYCOPY, /* 16 */
	VACCEL_F_MMULT, /* 17 */
	VACCEL_F_PARALLEL, /* 18 */
	VACCEL_F_VECTORADD, /* 19 */
	VACCEL_EXEC_WITH_RESOURCE, /* 20 */
	VACCEL_TORCH_JITLOAD_FORWARD, /* 21 */
	VACCEL_TORCH_SGEMM, /* 22 */
	VACCEL_OPENCV, /* 23 */
	VACCEL_TFLITE_SESSION_LOAD, /* 24 */
	VACCEL_TFLITE_SESSION_RUN, /* 25 */
	VACCEL_TFLITE_SESSION_DELETE, /* 26 */
	VACCEL_FUNCTIONS_NR
};

static const char *vaccel_op_name[] = {
	"noop",
	"sgemm",
	"image classification",
	"image detection",
	"image segmentation",
	"image pose estimation",
	"image depth estimation",
	"exec",
	"TensorFlow model create",
	"TensorFlow model destroy",
	"TensorFlow model register",
	"TensorFlow model unregister",
	"TensorFlow session load",
	"TensorFlow session run",
	"TensorFlow session delete",
	"MinMax",
	"Array copy",
	"Matrix multiplication",
	"Parallel acceleration",
	"Vector Add",
	"Exec with resource",
	"Torch jitload_forward function",
	"Torch SGEMM",
	"OpenCV Generic",
	"TensorFlow Lite session load",
	"TensorFlow Lite session run",
	"TensorFlow Lite session delete",
	"Functions NR",
};

static inline const char *vaccel_op_type_str(enum vaccel_op_type op_type)
{
	return vaccel_op_name[op_type];
}

#ifdef __cplusplus
}
#endif
