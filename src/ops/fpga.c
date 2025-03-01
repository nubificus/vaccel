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

typedef int (*fpga_arraycopy_fn_t)(struct vaccel_session *sess, int array[],
				   int out_array[], size_t len_array);

int vaccel_fpga_arraycopy(struct vaccel_session *sess, int array[],
			  int out_array[], size_t len_array)
{
	int ret;

	if (!sess)
		return VACCEL_EINVAL;

	vaccel_debug(
		"session:%" PRId64
		" Looking for plugin implementing fpga_arraycopy operation",
		sess->id);

	vaccel_prof_region_start(&fpga_arraycopy_op_stats);

	fpga_arraycopy_fn_t plugin_fpga_arraycopy =
		plugin_get_op_func(VACCEL_OP_FPGA_ARRAYCOPY, sess->hint);
	if (!plugin_fpga_arraycopy) {
		ret = VACCEL_ENOTSUP;
		goto out;
	}

	ret = plugin_fpga_arraycopy(sess, array, out_array, len_array);

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
	int *array = (int *)read[0].buf;
	size_t len_array = (size_t)read[0].size / sizeof(array[0]);
	int *out_array = (int *)write[0].buf;

	return vaccel_fpga_arraycopy(sess, array, out_array, len_array);
}

static struct vaccel_prof_region fpga_mmult_op_stats =
	VACCEL_PROF_REGION_INIT("vaccel_fpga_mmult_op");

typedef int (*fpga_mmult_fn_t)(struct vaccel_session *sess, float A_array[],
			       float B_array[], float C_array[], size_t lenA);

int vaccel_fpga_mmult(struct vaccel_session *sess, float A_array[],
		      float B_array[], float C_array[], size_t lenA)
{
	int ret;

	if (!sess)
		return VACCEL_EINVAL;

	vaccel_debug("session:%" PRId64
		     " Looking for plugin implementing fpga_mmult operation",
		     sess->id);

	vaccel_prof_region_start(&fpga_mmult_op_stats);

	fpga_mmult_fn_t plugin_fpga_mmult =
		plugin_get_op_func(VACCEL_OP_FPGA_MMULT, sess->hint);
	if (!plugin_fpga_mmult) {
		ret = VACCEL_ENOTSUP;
		goto out;
	}

	ret = plugin_fpga_mmult(sess, A_array, B_array, C_array, lenA);

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
	float *A_array = (float *)read[0].buf;
	float *B_array = (float *)read[1].buf;
	size_t lenA = (size_t)read[0].buf;
	float *C_array = (float *)write[0].buf;

	return vaccel_fpga_mmult(sess, A_array, B_array, C_array, lenA);
}

static struct vaccel_prof_region fpga_parallel_op_stats =
	VACCEL_PROF_REGION_INIT("vaccel_fpga_parallel_op");

typedef int (*fpga_parallel_fn_t)(struct vaccel_session *sess, float A_array[],
				  float B_array[], float add_output[],
				  float mult_output[], size_t len_a);

int vaccel_fpga_parallel(struct vaccel_session *sess, float A_array[],
			 float B_array[], float add_output[],
			 float mult_output[], size_t len_a)
{
	int ret;

	if (!sess)
		return VACCEL_EINVAL;

	vaccel_debug("session:%" PRId64
		     " Looking for plugin implementing fpga_parallel operation",
		     sess->id);

	vaccel_prof_region_start(&fpga_parallel_op_stats);

	fpga_parallel_fn_t plugin_fpga_parallel =
		plugin_get_op_func(VACCEL_OP_FPGA_PARALLEL, sess->hint);
	if (!plugin_fpga_parallel) {
		ret = VACCEL_ENOTSUP;
		goto out;
	}

	ret = plugin_fpga_parallel(sess, A_array, B_array, add_output,
				   mult_output, len_a);

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
	float *A_array = (float *)read[0].buf;
	float *B_array = (float *)read[1].buf;
	float *add_output = (float *)write[0].buf;
	float *mult_output = (float *)write[1].buf;
	size_t len_a = (size_t)read[0].size / sizeof(A_array[0]);

	return vaccel_fpga_parallel(sess, A_array, B_array, add_output,
				    mult_output, len_a);
}

static struct vaccel_prof_region fpga_vadd_op_stats =
	VACCEL_PROF_REGION_INIT("vaccel_fpga_vadd_op");

typedef int (*fpga_vadd_t)(struct vaccel_session *sess, float A[], float B[],
			   float C[], size_t len_a, size_t len_b);

int vaccel_fpga_vadd(struct vaccel_session *sess, float A[], float B[],
		     float C[], size_t len_a, size_t len_b)
{
	int ret;

	if (!sess)
		return VACCEL_EINVAL;

	vaccel_debug(
		"session:%" PRId64
		" Looking for plugin implementing fpga_vector_add operation",
		sess->id);

	vaccel_prof_region_start(&fpga_vadd_op_stats);

	fpga_vadd_t plugin_fpga_vadd =
		plugin_get_op_func(VACCEL_OP_FPGA_VECTORADD, sess->hint);
	if (!plugin_fpga_vadd) {
		ret = VACCEL_ENOTSUP;
		goto out;
	}

	ret = plugin_fpga_vadd(sess, A, B, C, len_a, len_b);

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
	float *A = (float *)read[0].buf;
	float *B = (float *)read[1].buf;
	float *C = (float *)write[0].buf;
	size_t len_a = (size_t)read[0].size / sizeof(A[0]);
	size_t len_b = (size_t)read[1].size / sizeof(B[0]);

	return vaccel_fpga_vadd(sess, A, B, C, len_a, len_b);
}

__attribute__((constructor)) static void vaccel_ops_init(void)
{
}

__attribute__((destructor)) static void vaccel_ops_fini(void)
{
	vaccel_prof_region_print(&fpga_arraycopy_op_stats);
	vaccel_prof_region_release(&fpga_arraycopy_op_stats);

	vaccel_prof_region_print(&fpga_mmult_op_stats);
	vaccel_prof_region_release(&fpga_mmult_op_stats);

	vaccel_prof_region_print(&fpga_parallel_op_stats);
	vaccel_prof_region_release(&fpga_parallel_op_stats);

	vaccel_prof_region_print(&fpga_vadd_op_stats);
	vaccel_prof_region_release(&fpga_vadd_op_stats);
}
