#include "genop.h"
#include "vaccel_ops.h"

#include "blas.h"
#include "image.h"
#include "noop.h"

#include <session.h>
#include <common.h>
#include <log.h>

typedef int (*unpack_func_t)(
	struct vaccel_session *sess,
	struct vaccel_arg *read,
	int nr_read,
	struct vaccel_arg *write,
	int nr_write
);

unpack_func_t callbacks[VACCEL_FUNCTIONS_NR] = {
	vaccel_noop_unpack,
	vaccel_sgemm_unpack,
	vaccel_image_classification_unpack,
	vaccel_image_detection_unpack,
	vaccel_image_segmentation_unpack,
};

int vaccel_genop(struct vaccel_session *sess, struct vaccel_arg  *read,
		int nr_read, struct vaccel_arg *write, int nr_write)
{
	if (!nr_read) {
		vaccel_error("Calling genop without op type");
		return VACCEL_EINVAL;
	}

	uint8_t op = (uint8_t)(uintptr_t)read[0].buf;
	if (op >= VACCEL_FUNCTIONS_NR) {
		vaccel_error("Invalid operation type: %u", op);
		return VACCEL_EINVAL;
	}

	return callbacks[op](sess, &read[1], nr_read - 1, write, nr_write);
}
