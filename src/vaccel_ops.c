#include "vaccel_ops.h"
#include "plugin.h"
#include "common.h"
#include "log.h"
#include "session.h"

/* export supported function as types for the rest of the runtime */
typedef typeof(vaccel_noop) noop_t;
typedef typeof(vaccel_sgemm) sgemm_t;

int vaccel_noop(struct vaccel_session *sess)
{
	if (!sess)
		return VACCEL_EINVAL;

	vaccel_debug("session:%u Looking for plugin implementing noop",
			sess->session_id);

	//Get implementation
	noop_t *plugin_op = get_plugin_op(VACCEL_NO_OP);
	if (!plugin_op)
		return VACCEL_ENOTSUP;

	return plugin_op(sess);
}
