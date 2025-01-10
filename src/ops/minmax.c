// SPDX-License-Identifier: Apache-2.0

#include "minmax.h"
#include "arg.h"
#include "error.h"
#include "log.h"
#include "op.h"
#include "plugin.h"
#include "session.h"
#include <inttypes.h>
#include <stdint.h>

typedef int (*minmax_fn_t)(struct vaccel_session *sess, const double *indata,
			   int ndata, int low_threshold, int high_threshold,
			   double *outdata, double *min, double *max);

int vaccel_minmax(struct vaccel_session *sess, const double *indata, int ndata,
		  int low_threshold, int high_threshold, double *outdata,
		  double *min, double *max)
{
	if (!sess)
		return VACCEL_EINVAL;

	vaccel_debug("session:%" PRId64 " Looking for plugin implementing %s",
		     sess->id, vaccel_op_type_to_str(VACCEL_OP_MINMAX));

	minmax_fn_t plugin_minmax =
		plugin_get_op_func(VACCEL_OP_MINMAX, sess->hint);
	if (!plugin_minmax)
		return VACCEL_ENOTSUP;

	return plugin_minmax(sess, indata, ndata, low_threshold, high_threshold,
			     outdata, min, max);
}

int vaccel_minmax_unpack(struct vaccel_session *sess, struct vaccel_arg *read,
			 int nr_read, struct vaccel_arg *write, int nr_write)
{
	if (nr_read != 4) {
		vaccel_error("Wrong number of read arguments in MinMax: %d",
			     nr_read);
		return VACCEL_EINVAL;
	}

	if (nr_write != 3) {
		vaccel_error("Wrong number of write arguments in SGEMM: %d",
			     nr_write);
		return VACCEL_EINVAL;
	}

	double *indata = (double *)read[0].buf;
	int ndata = *(int *)read[1].buf;
	int low_threshold = *(int *)read[2].buf;
	int high_threshold = *(int *)read[3].buf;

	vaccel_info("number of data: %d\n", *(int *)read[1].buf);
	vaccel_info("number of data: %d\n", ndata);
	double *outdata = (double *)write[0].buf;
	double *min = (double *)write[1].buf;
	double *max = (double *)write[2].buf;

	return vaccel_minmax(sess, indata, ndata, low_threshold, high_threshold,
			     outdata, min, max);
}
