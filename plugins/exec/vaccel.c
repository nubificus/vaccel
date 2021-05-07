#include <stdio.h>
#include <vaccel.h>
#include <byteswap.h>
#include <dlfcn.h>

#if 0
struct vector_arg {
        size_t len;
        uint8_t *buf;
} __attribute__ ((packed));
#endif

static int noop(struct vaccel_session *session)
{
	vaccel_debug("Calling no-op for session %u", session->session_id);

	vaccel_debug("[exec] [noop] \n");

	return VACCEL_OK;
}

static int exec(struct vaccel_session *session, const char *library, const char
		*fn_symbol, void *read, size_t nr_read, void *write,
		size_t nr_write)
{
	int i = 0;
	void *dl;
	int (*fptr)(void *, size_t, void *, size_t);
	int ret;
	struct vaccel_arg *args;

	vaccel_debug("Calling exec for session %u", session->session_id);

	vaccel_debug("[exec] library: %s", library);
	dl = dlopen(library, RTLD_NOW | RTLD_GLOBAL);
	if (!dl) {
		vaccel_error("%s", dlerror());
		return VACCEL_EINVAL;
	}

	/* Get the function pointer based on the relevant symbol */
	vaccel_debug("[exec] symbol: %s", fn_symbol);
	fptr = (int (*)(void*, size_t, void*,size_t))dlsym(dl, fn_symbol);
	if (!fptr) {
		vaccel_error("%s", dlerror());
		return VACCEL_EINVAL;
	}


	ret = (*fptr)(read, nr_read, write, nr_write);
	if (ret)
		return VACCEL_ENOEXEC;

	return VACCEL_OK;
}

struct vaccel_op ops[] = {
	VACCEL_OP_INIT(ops[0],VACCEL_NO_OP, noop),
	VACCEL_OP_INIT(ops[1],VACCEL_EXEC, exec),
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
	.name = "exec",
	.version = "0.1",
	.init = init,
	.fini = fini
)
