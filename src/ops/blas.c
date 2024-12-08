// SPDX-License-Identifier: Apache-2.0

#include "arg.h"
#include "blas.h"
#include "error.h"
#include "log.h"
#include "op.h"
#include "plugin.h"
#include "prof.h"
#include "session.h"
#include <inttypes.h>
#include <stdint.h>

struct vaccel_prof_region blas_op_stats =
	VACCEL_PROF_REGION_INIT("vaccel_blas_op");

int vaccel_sgemm(struct vaccel_session *sess, long long int m, long long int n,
		 long long int k, float alpha, float *a, long long int lda,
		 float *b, long long int ldb, float beta, float *c,
		 long long int ldc)
{
	int ret;

	if (!sess)
		return VACCEL_EINVAL;

	vaccel_debug("session:%" PRId64
		     " Looking for plugin implementing BLAS SGEMM",
		     sess->id);

	vaccel_prof_region_start(&blas_op_stats);

	//Get implementation
	int (*plugin_op)(struct vaccel_session *sess, long long int m,
			 long long int n, long long int k, float alpha,
			 float *a, long long int lda, float *b,
			 long long int ldb, float beta, float *c,
			 long long int ldc) =
		get_plugin_op(VACCEL_BLAS_SGEMM, sess->hint);
	if (!plugin_op)
		return VACCEL_ENOTSUP;

	ret = plugin_op(sess, m, n, k, alpha, a, lda, b, ldb, beta, c, ldc);

	vaccel_prof_region_stop(&blas_op_stats);

	return ret;
}

int vaccel_sgemm_unpack(struct vaccel_session *sess, struct vaccel_arg *read,
			int nr_read, struct vaccel_arg *write, int nr_write)
{
	if (nr_read != 7) {
		vaccel_error("Wrong number of read arguments in SGEMM: %d",
			     nr_read);
		return VACCEL_EINVAL;
	}

	if (nr_write != 1) {
		vaccel_error("Wrong number of write arguments in SGEMM: %d",
			     nr_write);
		return VACCEL_EINVAL;
	}

	long long int m = *(long long int *)read[0].buf;
	long long int n = *(long long int *)read[1].buf;
	long long int k = *(long long int *)read[2].buf;
	float alpha = *(float *)read[3].buf;
	long long int lda = (long long int)read[4].size;
	float *a = (float *)read[4].buf;
	long long int ldb = (long long int)read[5].size;
	float *b = (float *)read[5].buf;
	float beta = *(float *)read[6].buf;

	long long int ldc = (long long int)write[0].size;
	float *c = (float *)write[0].buf;

	return vaccel_sgemm(sess, m, n, k, alpha, a, lda, b, ldb, beta, c, ldc);
}

__attribute__((constructor)) static void vaccel_ops_init(void)
{
}

__attribute__((destructor)) static void vaccel_ops_fini(void)
{
	vaccel_prof_region_print(&blas_op_stats);
}
