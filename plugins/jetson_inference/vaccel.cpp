#include <vaccel.h>
#include <plugin.h>
#include <vaccel_ops.h>
#include "operations.h"

struct vaccel_op ops[] = {
	VACCEL_OP_INIT(ops[0], VACCEL_IMG_CLASS, (void *)jetson_image_classification),
	VACCEL_OP_INIT(ops[1], VACCEL_IMG_DETEC, (void *)jetson_image_detect),
	VACCEL_OP_INIT(ops[2], VACCEL_IMG_SEGME, (void *)jetson_image_segment),
};

int jetson_init(void)
{
	int ret = register_plugin_functions(ops, sizeof(ops) / sizeof(ops[0]));
	if (!ret)
		return ret;

	return VACCEL_OK;
}

int jetson_finalize(void)
{
	return VACCEL_OK;
}

VACCEL_MODULE(
	.name = "jetson-inference",
	.version = "0.1",
	.init = jetson_init,
	.fini = jetson_finalize,
)
