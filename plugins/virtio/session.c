#include <vaccel.h>
#include <stdint.h>
#include <sys/ioctl.h>
#include <accel.h>
#include <string.h>

#include "ioctl.h"

int virtio_sess_init(struct vaccel_session *sess, uint32_t flags)
{
	struct accel_session session;

	memset(&session, 0, sizeof(session));
	int ret = dev_write(VACCEL_SESS_CREATE, &session);
	if (ret)
		return ret;

	/* Write the session id back to user */
	sess->session_id = session.id;

	return VACCEL_OK;
}

int virtio_sess_free(struct vaccel_session *sess)
{
	struct accel_session session;

	memset(&session, 0, sizeof(session));
	session.id = sess->session_id;
	return dev_write(VACCEL_SESS_DESTROY, &session);
}
