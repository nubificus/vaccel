#include <vaccel.h>
#include <backend.h>

#include "session.h"

int register_functions(struct vaccel_backend *backend)
{
	return VACCEL_OK;
}

int vaccel_backend_initialize(struct vaccel_backend *backend)
{
	int ret;

	ret = initialize_backend(backend, "virtio-accel");
	if (ret)
		return ret;

	/* We need to set these before registering the backend */
	backend->vaccel_sess_init = virtio_sess_init;
	backend->vaccel_sess_free = virtio_sess_free;

	ret = register_virtio_backend(backend);
	if (ret)
		goto cleanup;

	ret = register_functions(backend);
	if (ret)
		goto cleanup;

	return VACCEL_OK;

cleanup:
	cleanup_backend(backend);
	return ret;
}

int vaccel_backend_finalize(struct vaccel_backend *backend)
{
	return VACCEL_OK;
}
