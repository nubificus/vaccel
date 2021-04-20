#include "noop.h"
#include "error.h"
#include "plugin.h"
#include "log.h"
#include "vaccel_ops.h"
#include "genop.h"

#include "session.h"

int vaccel_noop(struct vaccel_session *sess)
{
	if (!sess)
		return VACCEL_EINVAL;

	vaccel_debug("session:%u Looking for plugin implementing noop",
			sess->session_id);

	//Get implementation
	int (*plugin_op)() = get_plugin_op(VACCEL_NO_OP);
	if (!plugin_op)
		return VACCEL_ENOTSUP;

	return plugin_op(sess);
}

int vaccel_noop_unpack(struct vaccel_session *sess,
		struct vaccel_arg *read, int nr_read,
		struct vaccel_arg *write, int nr_write)
{
	if (nr_read || read) {
		vaccel_error("Wrong number of read arguments in noop: %d",
				nr_read);
		return VACCEL_EINVAL;
	}

	if (nr_write || write) {
		vaccel_error("Wrong number of write arguments in noop: %d",
				nr_write);
		return VACCEL_EINVAL;
	}

	return vaccel_noop(sess);
}
