#include <stdio.h>
#include <plugin.h>

#include "log.h"

static int noop(struct vaccel_session *session)
{
	vaccel_debug("Calling no-op for session %u", session->session_id);
	return VACCEL_OK;
}

static int genop(struct vaccel_session *session, void *out_args, void *in_args,
		size_t out_nargs, size_t in_nargs)
{
	int i = 0;
	vaccel_debug("Calling do-op for session %u", session->session_id);

	vaccel_debug("[noop] [genop] in_nargs: %d, out_nargs: %d\n", in_nargs, out_nargs);

	return VACCEL_OK;
}

struct vaccel_op ops[] = {
        VACCEL_OP_INIT(ops[0], VACCEL_NO_OP, noop),
        VACCEL_OP_INIT(ops[1], VACCEL_BLAS_SGEMM, noop),
        VACCEL_OP_INIT(ops[2], VACCEL_IMG_CLASS, noop),
        VACCEL_OP_INIT(ops[3], VACCEL_IMG_DETEC, noop),
        VACCEL_OP_INIT(ops[4], VACCEL_IMG_SEGME, noop),
        VACCEL_OP_INIT(ops[5], VACCEL_GEN_OP, genop),
};

static int init(void)
{
        return register_plugin_functions(ops, sizeof(ops) / sizeof(ops[0]));
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
