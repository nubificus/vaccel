#ifndef __NOOP_H__
#define __NOOP_H__

#include "include/ops/noop.h"

struct vaccel_session;
struct vaccel_arg;

int vaccel_noop_unpack(struct vaccel_session *sess,
		struct vaccel_arg *read, int nr_read,
		struct vaccel_arg *write, int nr_write);

#endif /* __NOOP_H__ */
