// SPDX-License-Identifier: Apache-2.0

#include "fpga.h"
#include "arg.h"
#include "error.h"
#include "log.h"
#include "op.h"
#include "plugin.h"
#include "prof.h"
#include "session.h"
#include <inttypes.h>
#include <stdint.h>

static struct vaccel_prof_region fpga_arraycopy_op_stats =
	VACCEL_PROF_REGION_INIT("vaccel_fpga_arraycopy_op");

typedef int (*fpga_arraycopy_fn_t)(struct vaccel_session *sess, int a[],
				   int out_a[], size_t len_a);

int vaccel_fpga_arraycopy(struct vaccel_session *sess, int a[], int out_a[],
			  size_t len_a)
{
	int ret;

	if (!sess)
		return VACCEL_EINVAL;

	vaccel_op_type_t op_type = VACCEL_OP_FPGA_ARRAYCOPY;
	op_debug_plugin_lookup(sess, op_type);

	vaccel_prof_region_start(&fpga_arraycopy_op_stats);

	fpga_arraycopy_fn_t plugin_fpga_arraycopy =
		plugin_get_op_func(op_type, sess->hint);
	if (!plugin_fpga_arraycopy) {
		ret = VACCEL_ENOTSUP;
		goto out;
	}

	ret = plugin_fpga_arraycopy(sess, a, out_a, len_a);

out:
	vaccel_prof_region_stop(&fpga_arraycopy_op_stats);

	return ret;
}

int vaccel_fpga_arraycopy_unpack(struct vaccel_session *sess,
				 struct vaccel_arg *read, int nr_read,
				 struct vaccel_arg *write, int nr_write)
{
	if (nr_read != 1) {
		vaccel_error(
			"Wrong number of read arguments in fpga_arraycopy: %d",
			nr_read);
		return VACCEL_EINVAL;
	}

	if (nr_write != 1) {
		vaccel_error(
			"Wrong number of write arguments in fpga_arraycopy: %d",
			nr_write);
		return VACCEL_EINVAL;
	}
	int *a = (int *)read[0].buf;
	size_t len_a = (size_t)read[0].size / sizeof(a[0]);

	int *out_a = (int *)write[0].buf;

	return vaccel_fpga_arraycopy(sess, a, out_a, len_a);
}

static struct vaccel_prof_region fpga_mmult_op_stats =
	VACCEL_PROF_REGION_INIT("vaccel_fpga_mmult_op");

typedef int (*fpga_mmult_fn_t)(struct vaccel_session *sess, float a[],
			       float b[], float c[], size_t len_a);

int vaccel_fpga_mmult(struct vaccel_session *sess, float a[], float b[],
		      float c[], size_t len_a)
{
	int ret;

	if (!sess)
		return VACCEL_EINVAL;

	vaccel_op_type_t op_type = VACCEL_OP_FPGA_MMULT;
	op_debug_plugin_lookup(sess, op_type);

	vaccel_prof_region_start(&fpga_mmult_op_stats);

	fpga_mmult_fn_t plugin_fpga_mmult =
		plugin_get_op_func(op_type, sess->hint);
	if (!plugin_fpga_mmult) {
		ret = VACCEL_ENOTSUP;
		goto out;
	}

	ret = plugin_fpga_mmult(sess, a, b, c, len_a);

out:
	vaccel_prof_region_stop(&fpga_mmult_op_stats);

	return ret;
}

int vaccel_fpga_mmult_unpack(struct vaccel_session *sess,
			     struct vaccel_arg *read, int nr_read,
			     struct vaccel_arg *write, int nr_write)
{
	if (nr_read != 2) {
		vaccel_error("Wrong number of read arguments in fpga_mmult: %d",
			     nr_read);
		return VACCEL_EINVAL;
	}

	if (nr_write != 1) {
		vaccel_error(
			"Wrong number of write arguments in fpga_mmult: %d",
			nr_write);
		return VACCEL_EINVAL;
	}
	float *a = (float *)read[0].buf;
	float *b = (float *)read[1].buf;
	size_t len_a = (size_t)read[0].buf;

	float *c = (float *)write[0].buf;

	return vaccel_fpga_mmult(sess, a, b, c, len_a);
}

static struct vaccel_prof_region fpga_parallel_op_stats =
	VACCEL_PROF_REGION_INIT("vaccel_fpga_parallel_op");

typedef int (*fpga_parallel_fn_t)(struct vaccel_session *sess, float a[],
				  float b[], float add_output[],
				  float mult_output[], size_t len_a);

int vaccel_fpga_parallel(struct vaccel_session *sess, float a[], float b[],
			 float add_output[], float mult_output[], size_t len_a)
{
	int ret;

	if (!sess)
		return VACCEL_EINVAL;

	vaccel_op_type_t op_type = VACCEL_OP_FPGA_PARALLEL;
	op_debug_plugin_lookup(sess, op_type);

	vaccel_prof_region_start(&fpga_parallel_op_stats);

	fpga_parallel_fn_t plugin_fpga_parallel =
		plugin_get_op_func(op_type, sess->hint);
	if (!plugin_fpga_parallel) {
		ret = VACCEL_ENOTSUP;
		goto out;
	}

	ret = plugin_fpga_parallel(sess, a, b, add_output, mult_output, len_a);

out:
	vaccel_prof_region_stop(&fpga_parallel_op_stats);

	return ret;
}

int vaccel_fpga_parallel_unpack(struct vaccel_session *sess,
				struct vaccel_arg *read, int nr_read,
				struct vaccel_arg *write, int nr_write)
{
	if (nr_read != 2) {
		vaccel_error(
			"Wrong number of read arguments in fpga_parallel: %d",
			nr_read);
		return VACCEL_EINVAL;
	}

	if (nr_write != 2) {
		vaccel_error(
			"Wrong number of write arguments in fpga_parallel: %d",
			nr_write);
		return VACCEL_EINVAL;
	}
	float *a = (float *)read[0].buf;
	float *b = (float *)read[1].buf;
	size_t len_a = (size_t)read[0].size / sizeof(a[0]);

	float *add_output = (float *)write[0].buf;
	float *mult_output = (float *)write[1].buf;

	return vaccel_fpga_parallel(sess, a, b, add_output, mult_output, len_a);
}

static struct vaccel_prof_region fpga_vadd_op_stats =
	VACCEL_PROF_REGION_INIT("vaccel_fpga_vadd_op");

typedef int (*fpga_vadd_t)(struct vaccel_session *sess, float a[], float b[],
			   float c[], size_t len_a, size_t len_b);

int vaccel_fpga_vadd(struct vaccel_session *sess, float a[], float b[],
		     float c[], size_t len_a, size_t len_b)
{
	int ret;

	if (!sess)
		return VACCEL_EINVAL;

	vaccel_op_type_t op_type = VACCEL_OP_FPGA_VECTORADD;
	op_debug_plugin_lookup(sess, op_type);

	vaccel_prof_region_start(&fpga_vadd_op_stats);

	fpga_vadd_t plugin_fpga_vadd = plugin_get_op_func(op_type, sess->hint);
	if (!plugin_fpga_vadd) {
		ret = VACCEL_ENOTSUP;
		goto out;
	}

	ret = plugin_fpga_vadd(sess, a, b, c, len_a, len_b);

out:
	vaccel_prof_region_stop(&fpga_vadd_op_stats);

	return ret;
}

int vaccel_fpga_vadd_unpack(struct vaccel_session *sess,
			    struct vaccel_arg *read, int nr_read,
			    struct vaccel_arg *write, int nr_write)
{
	if (nr_read != 2) {
		vaccel_error(
			"Wrong number of read arguments in fpga_vector_add: %d",
			nr_read);
		return VACCEL_EINVAL;
	}

	if (nr_write != 1) {
		vaccel_error(
			"Wrong number of write arguments in fpga_vector_add: %d",
			nr_write);
		return VACCEL_EINVAL;
	}
	float *a = (float *)read[0].buf;
	float *b = (float *)read[1].buf;
	size_t len_a = (size_t)read[0].size / sizeof(a[0]);
	size_t len_b = (size_t)read[1].size / sizeof(b[0]);

	float *c = (float *)write[0].buf;

	return vaccel_fpga_vadd(sess, a, b, c, len_a, len_b);
}

__attribute__((constructor)) static void vaccel_ops_init(void)
{
}

__attribute__((destructor)) static void vaccel_ops_fini(void)
{
	vaccel_prof_region_print(&fpga_arraycopy_op_stats);
	vaccel_prof_region_print(&fpga_mmult_op_stats);
	vaccel_prof_region_print(&fpga_parallel_op_stats);
	vaccel_prof_region_print(&fpga_vadd_op_stats);

	vaccel_prof_region_release(&fpga_arraycopy_op_stats);
	vaccel_prof_region_release(&fpga_mmult_op_stats);
	vaccel_prof_region_release(&fpga_parallel_op_stats);
	vaccel_prof_region_release(&fpga_vadd_op_stats);
}
