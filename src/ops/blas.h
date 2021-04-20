#ifndef __BLAS_H__
#define __BLAS_H__

#include <stddef.h>
#include <stdint.h>

#include "include/ops/blas.h"

struct vaccel_session;
struct vaccel_arg;

int vaccel_sgemm_unpack(struct vaccel_session *sess, struct vaccel_arg *read,
		int nr_read, struct vaccel_arg *write, int nr_write);

#endif /* __BLAS_H__ */
