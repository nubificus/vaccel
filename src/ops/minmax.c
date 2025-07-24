// SPDX-License-Identifier: Apache-2.0

#include "minmax.h"
#include "arg.h"
#include "error.h"
#include "log.h"
#include "op.h"
#include "plugin.h"
#include "prof.h"
#include "session.h"
#include <inttypes.h>
#include <stdint.h>

static struct vaccel_prof_region minmax_op_stats =
	VACCEL_PROF_REGION_INIT("vaccel_minmax_op");

typedef int (*minmax_fn_t)(struct vaccel_session *sess, const double *indata,
			   int ndata, int low_threshold, int high_threshold,
			   double *outdata, double *min, double *max);

int vaccel_minmax(struct vaccel_session *sess, const double *indata, int ndata,
		  int low_threshold, int high_threshold, double *outdata,
		  double *min, double *max)
{
	int ret;

	if (!sess)
		return VACCEL_EINVAL;

	vaccel_op_type_t op_type = VACCEL_OP_MINMAX;
	op_debug_plugin_lookup(sess, op_type);

	vaccel_prof_region_start(&minmax_op_stats);

	minmax_fn_t plugin_minmax = plugin_get_op_func(sess->plugin, op_type);
	if (!plugin_minmax) {
		ret = VACCEL_ENOTSUP;
		goto out;
	}

	ret = plugin_minmax(sess, indata, ndata, low_threshold, high_threshold,
			    outdata, min, max);

out:
	vaccel_prof_region_stop(&minmax_op_stats);

	return ret;
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

	double *outdata = (double *)write[0].buf;
	double *min = (double *)write[1].buf;
	double *max = (double *)write[2].buf;

	return vaccel_minmax(sess, indata, ndata, low_threshold, high_threshold,
			     outdata, min, max);
}

__attribute__((constructor)) static void vaccel_ops_init(void)
{
}

__attribute__((destructor)) static void vaccel_ops_fini(void)
{
	vaccel_prof_region_print(&minmax_op_stats);
	vaccel_prof_region_release(&minmax_op_stats);
}
