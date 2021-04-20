#ifndef __EXEC_OPS_H__
#define __EXEC_OPS_H__

#include <stddef.h>

#include "include/ops/exec.h"

struct vaccel_session;
struct vaccel_arg;

int vaccel_exec_unpack(struct vaccel_session *sess,
                struct vaccel_arg *read, int nr_read,
                struct vaccel_arg *write, int nr_write);

#endif /* __EXEC_OPS_H__ */
