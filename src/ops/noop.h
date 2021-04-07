#ifndef __VACCEL_NOOP_H__
#define __VACCEL_NOOP_H__

struct vaccel_session;
struct vaccel_arg;

int vaccel_noop(struct vaccel_session *sess);
int vaccel_noop_unpack(struct vaccel_session *sess,
		struct vaccel_arg *read, int nr_read,
		struct vaccel_arg *write, int nr_write);

#endif /* __VACCEL_NOOP_H__ */
