#ifndef __VACCEL_OPS_H__
#define __VACCEL_OPS_H__

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum vaccel_op_type {
	VACCEL_NO_OP = 0,
	VACCEL_BLAS_SGEMM,   /* 1 */
	VACCEL_IMG_CLASS,    /* 2 */
	VACCEL_IMG_DETEC,    /* 3 */
	VACCEL_IMG_SEGME,    /* 4 */
	VACCEL_EXEC,         /* 5 */
	VACCEL_TF_MODEL_NEW,        /* 6 */
	VACCEL_TF_MODEL_DESTROY,    /* 7 */
	VACCEL_TF_MODEL_REGISTER,   /* 8 */
	VACCEL_TF_MODEL_UNREGISTER, /* 9 */
	VACCEL_TF_MODEL_LOAD_GRAPH, /* 10 */
	VACCEL_TF_MODEL_RUN_GRAPH,  /* 11 */
	VACCEL_FUNCTIONS_NR
};

static const char *vaccel_op_name[] = {
	"noop",
	"sgemm",
	"image-classification",
	"image-detection",
	"image-segmentation",
	"exec",
	"TensorFlow model create",
	"TensorFlow model destroy",
	"TensorFlow model register",
	"TensorFlow model unregister",
	"TensorFlow model load graph",
	"TensorFlow model run graph",
};

static inline const char *vaccel_op_type_str(enum vaccel_op_type op_type)
{
	return vaccel_op_name[op_type];
}

#ifdef __cplusplus
}
#endif

#endif /* __VACCEL_OPS_H__ */
