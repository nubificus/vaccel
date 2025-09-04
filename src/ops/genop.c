// SPDX-License-Identifier: Apache-2.0

#include "genop.h"
#include "arg.h"
#include "blas.h"
#include "error.h"
#include "exec.h"
#include "fpga.h"
#include "image.h"
#include "log.h"
#include "minmax.h"
#include "noop.h"
#include "op.h"
#include "opencv.h"
#include "session.h"
#include <stdint.h>

typedef int (*unpack_func_t)(struct vaccel_session *sess,
			     struct vaccel_arg *read, int nr_read,
			     struct vaccel_arg *write, int nr_write);

static unpack_func_t callbacks[VACCEL_OP_MAX] = {
	[VACCEL_OP_NOOP] = vaccel_noop_unpack,
	[VACCEL_OP_BLAS_SGEMM] = vaccel_sgemm_unpack,
	[VACCEL_OP_IMAGE_CLASSIFY] = vaccel_image_classification_unpack,
	[VACCEL_OP_IMAGE_DETECT] = vaccel_image_detection_unpack,
	[VACCEL_OP_IMAGE_SEGMENT] = vaccel_image_segmentation_unpack,
	[VACCEL_OP_IMAGE_POSE] = vaccel_image_pose_unpack,
	[VACCEL_OP_IMAGE_DEPTH] = vaccel_image_depth_unpack,
	[VACCEL_OP_EXEC] = vaccel_exec_unpack,
	[VACCEL_OP_MINMAX] = vaccel_minmax_unpack,
	[VACCEL_OP_FPGA_ARRAYCOPY] = vaccel_fpga_arraycopy_unpack,
	[VACCEL_OP_FPGA_MMULT] = vaccel_fpga_mmult_unpack,
	[VACCEL_OP_FPGA_PARALLEL] = vaccel_fpga_parallel_unpack,
	[VACCEL_OP_FPGA_VECTORADD] = vaccel_fpga_vadd_unpack,
	[VACCEL_OP_EXEC_WITH_RESOURCE] = vaccel_exec_with_res_unpack,
	[VACCEL_OP_OPENCV] = vaccel_opencv_unpack,
};

int vaccel_genop(struct vaccel_session *sess, struct vaccel_arg *read,
		 int nr_read, struct vaccel_arg *write, int nr_write)
{
	if (nr_read < 1) {
		vaccel_error("Missing operation type");
		return VACCEL_EINVAL;
	}

	struct vaccel_arg_array read_args;
	int ret = vaccel_arg_array_wrap(&read_args, read, nr_read);
	if (ret) {
		vaccel_error("Failed to parse genop read args");
		return VACCEL_EINVAL;
	}

	uint8_t u_op_type;
	ret = vaccel_arg_array_get_uint8(&read_args, &u_op_type);
	if (ret) {
		vaccel_error("Failed to unpack operation type for genop");
		return VACCEL_EINVAL;
	}

	vaccel_op_type_t op_type = (vaccel_op_type_t)u_op_type;
	if (!op_type || op_type >= VACCEL_OP_MAX) {
		vaccel_error("Invalid operation type");
		return VACCEL_EINVAL;
	}

	if (!callbacks[op_type]) {
		vaccel_error("Operation not implemented for %s",
			     vaccel_op_type_to_str(op_type));
		return VACCEL_ENOTSUP;
	}

	return callbacks[op_type](sess, &read[1], nr_read - 1, write, nr_write);
}
