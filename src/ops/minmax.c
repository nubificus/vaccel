#include "session.h"
#include "vaccel_ops.h"
#include "genop.h"
#include "log.h"
#include "error.h"
#include "plugin.h"

#include "minmax.h"

int vaccel_minmax(
	struct vaccel_session *sess,
	const double *indata, int ndata,
	int low_threshold, int high_threshold,
	double *outdata,
	double *min, double *max
) {
	if (!sess)
		return VACCEL_EINVAL;

	vaccel_debug("session:%u Looking for plugin implementing %s",
				sess->session_id,
				vaccel_op_type_str(VACCEL_MINMAX));

	int (*plugin_op)(
		struct vaccel_session *sess,
		const double *indata, int ndata,
		int low_threshold, int high_threshold,
		double *outdata,
		double *min, double *max
	) = get_plugin_op(VACCEL_MINMAX);
	if (!plugin_op)
		return VACCEL_ENOTSUP;

	return plugin_op(sess, indata, ndata, low_threshold, high_threshold,
			outdata, min, max);
}
