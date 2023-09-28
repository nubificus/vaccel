/*
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "genop.h"
#include "vaccel_ops.h"

#include "blas.h"
#include "minmax.h"
#include "exec.h"
#include "image.h"
#include "noop.h"
#include "fpga.h"
#include "opencv.h"

#include <session.h>
#include <error.h>
#include <log.h>
#include "torch.h"

typedef int (*unpack_func_t)(
	struct vaccel_session *sess,
	struct vaccel_arg *read,
	int nr_read,
	struct vaccel_arg *write,
	int nr_write
);

unpack_func_t callbacks[VACCEL_FUNCTIONS_NR] = {
	vaccel_noop_unpack,			/* 0 */
	vaccel_sgemm_unpack,			/* 1 */
	vaccel_image_classification_unpack,	/* 2 */
	vaccel_image_detection_unpack,		/* 3 */
	vaccel_image_segmentation_unpack,	/* 4 */
	vaccel_image_pose_unpack,		/* 5 */
	vaccel_image_depth_unpack,		/* 6 */
	vaccel_exec_unpack,			/* 7 */
	vaccel_noop_unpack,			/* 8 */
	vaccel_noop_unpack,			/* 9 */
	vaccel_noop_unpack,			/* 10 */
	vaccel_noop_unpack,			/* 11 */
	vaccel_noop_unpack,			/* 12 */
	vaccel_noop_unpack,			/* 13 */
	vaccel_noop_unpack,			/* 14 */
	vaccel_minmax_unpack,			/* 15 */
	vaccel_fpga_arraycopy_unpack,		/* 16 */
	vaccel_fpga_mmult_unpack,		/* 17 */
	vaccel_fpga_parallel_unpack,		/* 18 */
	vaccel_fpga_vadd_unpack,		/* 19 */
	vaccel_exec_with_res_unpack,		/* 20 */
	vaccel_noop_unpack,			/* 21 */
	vaccel_noop_unpack,			/* 22 */
	vaccel_opencv_unpack,			/* 23 */
};

int vaccel_genop(struct vaccel_session *sess, struct vaccel_arg  *read,
		int nr_read, struct vaccel_arg *write, int nr_write)
{
	if (!nr_read) {
		vaccel_error("Calling genop without op type");
		return VACCEL_EINVAL;
	}

	enum vaccel_op_type op = *(enum vaccel_op_type *)read[0].buf;
	if (op >= VACCEL_FUNCTIONS_NR) {
		vaccel_error("Invalid operation type: %u", op);
		return VACCEL_EINVAL;
	}

	return callbacks[op](sess, &read[1], nr_read - 1, write, nr_write);
}
