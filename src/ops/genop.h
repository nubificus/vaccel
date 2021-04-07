#ifndef __VACCEL_GENOP_H__
#define __VACCEL_GENOP_H__

#include <stdint.h>

#include "vaccel_ops.h"

struct vaccel_session;

struct vaccel_arg {
	uint32_t size;
	void *buf;
};

/* Call one of the supported functions, given an op code and a set of arbitrary
 * arguments */
int vaccel_genop(struct vaccel_session *sess, struct vaccel_arg *read,
		int nr_read, struct vaccel_arg *write, int nr_write);

#endif /* __VACCEL_GENOP_H__ */
