#ifndef __VACCEL_BLAS_H__
#define __VACCEL_BLAS_H__

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

struct vaccel_session;

int vaccel_sgemm(struct vaccel_session *sess, uint32_t k, uint32_t m,
		uint32_t n, size_t len_a, size_t len_b, size_t len_c,
		float *a, float *b, float *c);

#ifdef __cplusplus
}
#endif

#endif /* __VACCEL_BLAS_H__ */
