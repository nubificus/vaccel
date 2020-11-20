#ifndef __VACCEL_VIRTIO_SESSION_H__
#define __VACCEL_VIRTIO_SESSION_H__

#include <stdint.h>

struct vaccel_session;

int virtio_sess_init(struct vaccel_session *sess, uint32_t flags);
int virtio_sess_free(struct vaccel_session *sess);

#endif /* __VACCEL_VIRTIO_SESSION_H__ */
