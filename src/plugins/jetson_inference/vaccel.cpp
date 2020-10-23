#include <vaccel.h>
#include <backend.h>
#include <vaccel_ops.h>
#include "operations.h"

extern "C" int vaccel_backend_initialize(struct vaccel_backend *backend)
{
	int ret;

	ret = initialize_backend(backend, "jetson_inference");
	if (ret)
		return ret;

	ret = register_backend(backend);
	if (ret)
		return ret;

	ret = register_backend_function(backend, VACCEL_IMG_CLASS,
			(void *)jetson_image_classification);
	if (ret)
		goto unregister;

	ret = register_backend_function(backend, VACCEL_IMG_DETEC,
			(void *)jetson_image_detect);
	if (ret)
		goto unregister;

	ret = register_backend_function(backend, VACCEL_IMG_SEGME,
			(void *)jetson_image_segment);
	if (ret)
		goto unregister;

	return VACCEL_OK;

unregister:
	cleanup_backend(backend);
	return ret;
}

extern "C" int vaccel_backend_finalize(struct vaccel_backend *backend)
{
	return VACCEL_OK;
}
