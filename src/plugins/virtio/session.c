#include <vaccel.h>
#include <stdint.h>
#include <sys/ioctl.h>
#include <accel.h>
#include <string.h>

#include "ioctl.h"

int virtio_sess_init(struct vaccel_session *sess, uint32_t flags)
{
	struct accel_session session;
	struct accel_gen_op_arg args = {
		sizeof(sess->session_id),
		(char *)&sess->session_id
	};

	memset(&session, 0, sizeof(session));
	session.u.gen.out_nr = 1;
	session.u.gen.out = &args;

	return dev_write(ACCIOC_GEN_SESS_CREATE, &session);
}

int virtio_sess_free(struct vaccel_session *sess)
{
	struct accel_session session;

	memset(&session, 0, sizeof(session));
	return dev_write(ACCIOC_GEN_SESS_DESTROY, &session);
}
