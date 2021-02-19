#include <vaccel.h>
#include <session.h>
#include <stdint.h>
#include <sys/ioctl.h>
#include <accel.h>
#include <string.h>

#include "ioctl.h"
#include "log.h"

int virtio_sess_init(struct vaccel_session *sess, uint32_t flags)
{
	struct accel_session session;

	memset(&session, 0, sizeof(session));
	int ret = dev_write(VACCEL_SESS_CREATE, &session);
	if (ret)
		return ret;

	/* Write the session id back to user */
	sess->session_id = session.id;

	vaccel_debug("session:%u New session", sess->session_id);

	return VACCEL_OK;
}

int virtio_sess_free(struct vaccel_session *sess)
{
	struct accel_session session;
	int ret;

	memset(&session, 0, sizeof(session));
	session.id = sess->session_id;
	ret = dev_write(VACCEL_SESS_DESTROY, &session);

	vaccel_debug("session:%u Free session", sess->session_id);

	return ret;
}
