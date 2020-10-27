#include "vaccel.h"
#include "backend.h"

#include <stdint.h>

int vaccel_sess_init(struct vaccel_session *sess, uint32_t flags)
{
	/* if we're using virtio as a backend offload the session initialization
	 * to the host */
	struct vaccel_backend *virtio = get_virtio_backend();
	if (virtio)
		return virtio->vaccel_sess_init(sess, flags);


	return VACCEL_OK;
}

int vaccel_sess_free(struct vaccel_session *sess)
{
	/* if we're using virtio as a backend offload the session cleanup to the
	 * host */
	struct vaccel_backend *virtio = get_virtio_backend();
	if (virtio)
		return virtio->vaccel_sess_free(sess);

	return VACCEL_OK;
}
