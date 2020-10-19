#include <stdio.h>

#include <vaccel.h>
#include "backend.h"

static int noop(struct vaccel_session *session)
{
	fprintf(stdout, "Calling no-op for session %u", session->session_id);
	return VACCEL_OK;
}

int vaccel_backend_init(struct vaccel_backend *backend)
{
	int ret;

	ret = initialize_backend(backend, "noop_debug");
	if (!ret)
		return ret;

	ret = register_backend(backend);
	if (!ret)
		goto cleanup;

	ret = register_backend_function(backend, VACCEL_NO_OP, noop);
	if (!ret)
		goto unregister;

	return VACCEL_OK;

unregister:
	unregister_backend(backend);
cleanup:
	cleanup_backend(backend);
	return ret;
}

int vaccel_backend_fini(struct vaccel_backend *backend)
{
	return VACCEL_OK;
}
