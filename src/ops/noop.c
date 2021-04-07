#include "noop.h"
#include "common.h"
#include "plugin.h"
#include "log.h"
#include "vaccel_ops.h"

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
