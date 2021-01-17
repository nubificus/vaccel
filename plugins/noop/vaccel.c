#include <stdio.h>
#include <plugin.h>

static int noop(struct vaccel_session *session)
{
	fprintf(stdout, "Calling no-op for session %u", session->session_id);
	return VACCEL_OK;
}

struct vaccel_op op = VACCEL_OP_INIT(op, VACCEL_NO_OP, noop);

static int init(void)
{
	return register_plugin_function(&op);
}

static int fini(void)
{
	return VACCEL_OK;
}

VACCEL_MODULE(
	.name = "noop",
	.version = "0.1",
	.init = init,
	.fini = fini
)
