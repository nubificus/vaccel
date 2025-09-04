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
#include <stddef.h>
#include <stdint.h>

static struct vaccel_prof_region blas_op_stats =
	VACCEL_PROF_REGION_INIT("vaccel_blas_op");

typedef int (*sgemm_fn_t)(struct vaccel_session *sess, int64_t m, int64_t n,
			  int64_t k, float alpha, float *a, int64_t lda,
			  float *b, int64_t ldb, float beta, float *c,
			  int64_t ldc);

int vaccel_sgemm(struct vaccel_session *sess, int64_t m, int64_t n, int64_t k,
		 float alpha, float *a, int64_t lda, float *b, int64_t ldb,
		 float beta, float *c, int64_t ldc)
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
		vaccel_error("Wrong number of read arguments in sgemm: %d",
			     nr_read);
		return VACCEL_EINVAL;
	}

	if (nr_write != 1) {
		vaccel_error("Wrong number of write arguments in sgemm: %d",
			     nr_write);
		return VACCEL_EINVAL;
	}

	struct vaccel_arg_array read_args;
	int ret = vaccel_arg_array_wrap(&read_args, read, nr_read);
	if (ret) {
		vaccel_error("Failed to parse sgemm read args");
		return VACCEL_EINVAL;
	}

	struct vaccel_arg_array write_args;
	ret = vaccel_arg_array_wrap(&write_args, write, nr_write);
	if (ret) {
		vaccel_error("Failed to parse sgemm write args");
		return VACCEL_EINVAL;
	}

	int64_t m;
	ret = vaccel_arg_array_get_int64(&read_args, &m);
	if (ret) {
		vaccel_error("Failed to unpack m arg for sgemm");
		return VACCEL_EINVAL;
	}

	int64_t n;
	ret = vaccel_arg_array_get_int64(&read_args, &n);
	if (ret) {
		vaccel_error("Failed to unpack n arg for sgemm");
		return VACCEL_EINVAL;
	}

	int64_t k;
	ret = vaccel_arg_array_get_int64(&read_args, &k);
	if (ret) {
		vaccel_error("Failed to unpack k arg for sgemm");
		return VACCEL_EINVAL;
	}

	float alpha;
	ret = vaccel_arg_array_get_float(&read_args, &alpha);
	if (ret) {
		vaccel_error("Failed to unpack alpha arg for sgemm");
		return VACCEL_EINVAL;
	}

	float *a;
	ret = vaccel_arg_array_get_float_array(&read_args, &a, NULL);
	if (ret) {
		vaccel_error("Failed to unpack a arg for sgemm");
		return VACCEL_EINVAL;
	}

	int64_t lda;
	ret = vaccel_arg_array_get_int64(&read_args, &lda);
	if (ret) {
		vaccel_error("Failed to unpack lda arg for sgemm");
		return VACCEL_EINVAL;
	}

	float *b;
	ret = vaccel_arg_array_get_float_array(&read_args, &b, NULL);
	if (ret) {
		vaccel_error("Failed to unpack b arg for sgemm");
		return VACCEL_EINVAL;
	}

	int64_t ldb;
	ret = vaccel_arg_array_get_int64(&read_args, &ldb);
	if (ret) {
		vaccel_error("Failed to unpack ldb arg for sgemm");
		return VACCEL_EINVAL;
	}

	float beta;
	ret = vaccel_arg_array_get_float(&read_args, &beta);
	if (ret) {
		vaccel_error("Failed to unpack beta arg for sgemm");
		return VACCEL_EINVAL;
	}

	int64_t ldc;
	ret = vaccel_arg_array_get_int64(&read_args, &ldc);
	if (ret) {
		vaccel_error("Failed to unpack ldc arg for sgemm");
		return VACCEL_EINVAL;
	}

	float *c;
	ret = vaccel_arg_array_get_float_array(&write_args, &c, NULL);
	if (ret) {
		vaccel_error("Failed to unpack c arg for sgemm");
		return VACCEL_EINVAL;
	}

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
