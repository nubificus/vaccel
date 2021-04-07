#ifndef __VACCEL_BLAS_H__
#define __VACCEL_BLAS_H__

#include <stddef.h>
#include <stdint.h>

struct vaccel_session;


int virtio_sgemm(struct vaccel_session *sess, uint32_t k, uint32_t m,
		uint32_t n, size_t len_a, size_t len_b, size_t len_c,
		float *a, float *b, float *c);

#endif /* __VACCEL_BLAS_H__ */
