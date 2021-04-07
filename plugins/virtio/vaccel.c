#include <ops/vaccel_ops.h>
#include <plugin.h>

#include "session.h"
#include "operations.h"

struct vaccel_op ops[] = {
	VACCEL_OP_INIT(ops[0], VACCEL_NO_OP, virtio_noop),
	VACCEL_OP_INIT(ops[1], VACCEL_BLAS_SGEMM, virtio_sgemm),
	VACCEL_OP_INIT(ops[2], VACCEL_IMG_CLASS, virtio_image_classification),
	VACCEL_OP_INIT(ops[3], VACCEL_IMG_DETEC, virtio_image_detection),
	VACCEL_OP_INIT(ops[4], VACCEL_IMG_SEGME, virtio_image_segmentation),
};

int virtio_init(void)
{
	int ret = register_plugin_functions(ops, sizeof(ops) / sizeof(ops[0]));
	if (!ret)
		return ret;

	return VACCEL_OK;
}

int virtio_finalize(void)
{
	return VACCEL_OK;
}

VACCEL_MODULE(
	.name = "virtio",
	.version = "0.1",
	.init = virtio_init,
	.fini = virtio_finalize,
	.sess_init = virtio_sess_init,
	.sess_free = virtio_sess_free,
)
