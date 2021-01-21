#include <vaccel.h>
#include <plugin.h>
#include <vaccel_ops.h>
#include "operations.h"

struct vaccel_op ops[] = {
	VACCEL_OP_INIT(ops[0], VACCEL_IMG_CLASS, (void *)coral_image_classification),
};

int coral_init(void)
{
	int ret = register_plugin_functions(ops, sizeof(ops) / sizeof(ops[0]));
	if (!ret)
		return ret;

	return VACCEL_OK;
}

int coral_finalize(void)
{
	return VACCEL_OK;
}

VACCEL_MODULE(
	.name = "google_coral",
	.version = "0.1",
	.init = coral_init,
	.fini = coral_finalize,
)
