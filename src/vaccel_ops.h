#ifndef __VACCEL_OPS_H__
#define __VACCEL_OPS_H__

#include <stddef.h>
#include <stdint.h>

#include "ml/ml_ops.h"

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

inline static const char *vaccel_op_type_str(uint8_t op_type)
{
	return vaccel_op_name[op_type];
}

struct vaccel_session;

/* vaccel supported operations */
int vaccel_noop(struct vaccel_session *sess);
int vaccel_sgemm(struct vaccel_session *sess,
		uint32_t k, uint32_t m, uint32_t n,
		size_t len_a, size_t len_b, size_t len_c,
		float *a, float *b, float *c);

#endif /* __VACCEL_OPS_H__ */
