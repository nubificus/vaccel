#ifndef __VACCEL_EXEC_OPS_H__
#define __VACCEL_EXEC_OPS_H__

#include <stddef.h>

struct vaccel_session;
struct vaccel_arg;

int vaccel_exec(struct vaccel_session *sess, const char *library,
                const char *fn_symbol, struct vaccel_arg *read,
                size_t nr_read, struct vaccel_arg *write, size_t nr_write);

int vaccel_exec_unpack(struct vaccel_session *sess,
                struct vaccel_arg *read, int nr_read,
                struct vaccel_arg *write, int nr_write);

#endif /* __VACCEL_EXEC_OPS_H__ */
