#include "fpga.h"
#include "error.h"
#include "plugin.h"
#include "log.h"
#include "vaccel_ops.h"
#include "genop.h"

#include "session.h"
int vaccel_fpga_arraycopy(struct vaccel_session *sess, int array[], int out_array[], size_t len_array)
{
	if (!sess)
		return VACCEL_EINVAL;

	vaccel_debug("session:%u Looking for plugin implementing fpga_arraycopy operation",
			sess->session_id);

	//Get implementation
	int (*plugin_op)() = get_plugin_op(VACCEL_F_ARRAYCOPY, sess->hint);
	if (!plugin_op)
		return VACCEL_ENOTSUP;

	return plugin_op(sess, array, out_array, len_array);
}

int vaccel_fpga_arraycopy_unpack(struct vaccel_session *sess, struct vaccel_arg *read,
		int nr_read, struct vaccel_arg *write, int nr_write)
{
	if (nr_read != 1) {
		vaccel_error("Wrong number of read arguments in fpga_arraycopy: %d",
				nr_read);
		return VACCEL_EINVAL;
	}

	if (nr_write != 1) {
		vaccel_error("Wrong number of write arguments in fpga_arraycopy: %d",
				nr_write);
		return VACCEL_EINVAL;
	}
	int *array = (int*)read[0].buf;
	size_t len_array = (size_t)read[0].size / sizeof(array[0]);
	int *out_array = (int*)write[0].buf;

	return vaccel_fpga_arraycopy(sess, array, out_array, len_array);
}






int vaccel_fpga_mmult(struct vaccel_session *sess, float A_array[], float B_array[], float C_array[], size_t lenA)
{
	if (!sess)
		return VACCEL_EINVAL;

	vaccel_debug("session:%u Looking for plugin implementing fpga_mmult operation",
			sess->session_id);

	//Get implementation
	int (*plugin_op)() = get_plugin_op(VACCEL_F_MMULT, sess->hint);
	if (!plugin_op)
		return VACCEL_ENOTSUP;

	return plugin_op(sess, A_array, B_array, C_array, lenA);
}

int vaccel_fpga_mmult_unpack(struct vaccel_session *sess, struct vaccel_arg *read,
		int nr_read, struct vaccel_arg *write, int nr_write)
{
	if (nr_read != 2) {
		vaccel_error("Wrong number of read arguments in fpga_mmult: %d",
				nr_read);
		return VACCEL_EINVAL;
	}

	if (nr_write != 1) {
		vaccel_error("Wrong number of write arguments in fpga_mmult: %d",
				nr_write);
		return VACCEL_EINVAL;
	}
	float *A_array = (float*)read[0].buf;
	float *B_array = (float*)read[1].buf;
    	size_t lenA = (size_t)read[0].buf;
    	float *C_array = (float*)write[0].buf;

	return vaccel_fpga_mmult(sess, A_array, B_array, C_array, lenA);
}





int vaccel_fpga_parallel(struct vaccel_session *sess, float A_array[], float B_array[], float add_output[], float mult_output[], size_t len_a)
{
	if (!sess)
		return VACCEL_EINVAL;

	vaccel_debug("session:%u Looking for plugin implementing fpga_parellel operation",
			sess->session_id);

	//Get implementation
	int (*plugin_op)() = get_plugin_op(VACCEL_F_PARALLEL, sess->hint);
	if (!plugin_op)
		return VACCEL_ENOTSUP;

	return plugin_op(sess, A_array, B_array, add_output, mult_output, len_a);
}

int vaccel_fpga_parallel_unpack(struct vaccel_session *sess, struct vaccel_arg *read,
		int nr_read, struct vaccel_arg *write, int nr_write)
{
	if (nr_read != 2) {
		vaccel_error("Wrong number of read arguments in fpga_parallel: %d",
				nr_read);
		return VACCEL_EINVAL;
	}

	if (nr_write != 2) {
		vaccel_error("Wrong number of write arguments in fpga_parallel: %d",
				nr_write);
		return VACCEL_EINVAL;
	}
	float *A_array      = (float*)read[0].buf;
	float *B_array      = (float*)read[1].buf;
    float *add_output   = (float*)write[0].buf;
    float *mult_output  = (float*)write[1].buf;
	size_t len_a 		= (size_t)read[0].size/sizeof(A_array[0]);


	return vaccel_fpga_parallel(sess, A_array, B_array, add_output, mult_output, len_a);

}





int vaccel_fpga_vadd(struct vaccel_session *sess, float A[], float B[], float C[], size_t len_a, size_t len_b)
{
	if (!sess)
		return VACCEL_EINVAL;

	vaccel_debug("session:%u Looking for plugin implementing fpga_vector_add operation",
			sess->session_id);

	//Get implementation
	int (*plugin_op)() = get_plugin_op(VACCEL_F_VECTORADD, sess->hint);
	if (!plugin_op)
		return VACCEL_ENOTSUP;

	return plugin_op(sess, A, B, C, len_a, len_b);
}

int vaccel_fpga_vadd_unpack(struct vaccel_session *sess, struct vaccel_arg *read,
		int nr_read, struct vaccel_arg *write, int nr_write)
{
	if (nr_read != 2) {
		vaccel_error("Wrong number of read arguments in fpga_vector_add: %d",
				nr_read);
		return VACCEL_EINVAL;
	}

	if (nr_write != 1) {
		vaccel_error("Wrong number of write arguments in fpga_vector_add: %d",
				nr_write);
		return VACCEL_EINVAL;
	}
	float *A      = (float*)read[0].buf;
	float *B      = (float*)read[1].buf;
    float *C      = (float*)write[0].buf;
	size_t len_a  = (size_t)read[0].size/sizeof(A[0]);
	size_t len_b  = (size_t)read[1].size/sizeof(B[0]);

    return vaccel_fpga_vadd(sess, A,B,C, len_a, len_b);
}
