#ifndef __VACCEL_OPS_H__
#define __VACCEL_OPS_H__

#include <stddef.h>
#include <stdint.h>

#define VACCEL_NO_OP        0
#define VACCEL_BLAS_SGEMM   1
#define VACCEL_IMG_CLASS    2
#define VACCEL_IMG_DETEC    3
#define VACCEL_IMG_SEGME    4
#define VACCEL_FUNCTIONS_NR 5

static const char *vaccel_op_name[] = {
	"noop",
	"sgemm",
	"image-classification",
	"image-detection",
	"image-segmentation",
};

static inline const char *vaccel_op_type_str(uint8_t op_type)
{
	return vaccel_op_name[op_type];
}

struct vaccel_arg {
	uint32_t size;
	void *buf;
};

#endif /* __VACCEL_OPS_H__ */
