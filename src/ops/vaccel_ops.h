#ifndef __VACCEL_OPS_H__
#define __VACCEL_OPS_H__

#include <stddef.h>
#include <stdint.h>

enum vaccel_op_type {
	VACCEL_NO_OP = 0,
	VACCEL_BLAS_SGEMM, 	/* 1 */
	VACCEL_IMG_CLASS, 	/* 2 */
	VACCEL_IMG_DETEC,    	/* 3 */
	VACCEL_IMG_SEGME,    	/* 4 */
	VACCEL_EXEC,		/* 5 */
	VACCEL_FUNCTIONS_NR
};

static const char *vaccel_op_name[] = {
	"noop",
	"sgemm",
	"image-classification",
	"image-detection",
	"image-segmentation",
	"exec",
};

static inline const char *vaccel_op_type_str(enum vaccel_op_type op_type)
{
	return vaccel_op_name[op_type];
}

#endif /* __VACCEL_OPS_H__ */
