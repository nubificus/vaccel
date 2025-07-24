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

static struct vaccel_prof_region blas_op_stats =
	VACCEL_PROF_REGION_INIT("vaccel_blas_op");

typedef int (*sgemm_fn_t)(struct vaccel_session *sess, long long int m,
			  long long int n, long long int k, float alpha,
			  float *a, long long int lda, float *b,
			  long long int ldb, float beta, float *c,
			  long long int ldc);

int vaccel_sgemm(struct vaccel_session *sess, long long int m, long long int n,
		 long long int k, float alpha, float *a, long long int lda,
		 float *b, long long int ldb, float beta, float *c,
		 long long int ldc)
{
	int ret;

	if (!sess)
		return VACCEL_EINVAL;

	vaccel_op_type_t op_type = VACCEL_OP_BLAS_SGEMM;
	op_debug_plugin_lookup(sess, op_type);

	vaccel_prof_region_start(&blas_op_stats);

	sgemm_fn_t plugin_sgemm = plugin_get_op_func(sess->plugin, op_type);
	if (!plugin_sgemm) {
		ret = VACCEL_ENOTSUP;
		goto out;
	}

	ret = plugin_sgemm(sess, m, n, k, alpha, a, lda, b, ldb, beta, c, ldc);

out:
	vaccel_prof_region_stop(&blas_op_stats);

	return ret;
}

int vaccel_sgemm_unpack(struct vaccel_session *sess, struct vaccel_arg *read,
			int nr_read, struct vaccel_arg *write, int nr_write)
{
	if (nr_read != 10) {
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
	float *a = (float *)read[4].buf;
	long long int lda = *(long long int *)read[5].buf;
	float *b = (float *)read[6].buf;
	long long int ldb = *(long long int *)read[7].buf;
	float beta = *(float *)read[8].buf;
	long long int ldc = *(long long int *)read[9].buf;

	float *c = (float *)write[0].buf;

	return vaccel_sgemm(sess, m, n, k, alpha, a, lda, b, ldb, beta, c, ldc);
}

__attribute__((constructor)) static void vaccel_ops_init(void)
{
}

__attribute__((destructor)) static void vaccel_ops_fini(void)
{
	vaccel_prof_region_print(&blas_op_stats);
	vaccel_prof_region_release(&blas_op_stats);
}
