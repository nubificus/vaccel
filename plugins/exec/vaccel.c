// SPDX-License-Identifier: Apache-2.0

#define _POSIX_C_SOURCE 200809L

#include <byteswap.h>
#include <dlfcn.h>
#include <stdio.h>
#include <vaccel.h>

#define exec_debug(fmt, ...) vaccel_debug("[exec] " fmt, ##__VA_ARGS__)
#define exec_error(fmt, ...) vaccel_error("[exec] " fmt, ##__VA_ARGS__)

#define exec_res_debug(fmt, ...) \
	vaccel_debug("[exec_with_resource] " fmt, ##__VA_ARGS__)
#define exec_res_error(fmt, ...) \
	vaccel_error("[exec_with_resource] " fmt, ##__VA_ARGS__)

static int noop(struct vaccel_session *session)
{
	exec_debug("Calling no-op for session %u", session->session_id);
	exec_debug("[noop]");

	return VACCEL_OK;
}

static int exec(struct vaccel_session *session, const char *library,
		const char *fn_symbol, void *read, size_t nr_read, void *write,
		size_t nr_write)
{
	void *dl;
	int (*fptr)(void *, size_t, void *, size_t);
	int ret;
	struct vaccel_arg *args;

	exec_debug("Calling exec for session %u", session->session_id);

	exec_debug("library: %s", library);
	dl = dlopen(library, RTLD_NOW);
	if (!dl) {
		exec_error("dlopen: %s", dlerror());
		return VACCEL_EINVAL;
	}

	/* Get the function pointer based on the relevant symbol */
	exec_debug("symbol: %s", fn_symbol);

	args = (struct vaccel_arg *)read;
	for (size_t i = 0; i < nr_read; i++) {
		exec_debug("read[%d].size: %u", i, args[i].size);
		exec_debug("read[%d].argtype: %u", i, args[i].argtype);
	}
	args = (struct vaccel_arg *)write;
	for (size_t i = 0; i < nr_write; i++) {
		exec_debug("write[%d].size: %u", i, args[i].size);
		exec_debug("write[%d].argtype: %u", i, args[i].argtype);
	}

	fptr = (int (*)(void *, size_t, void *, size_t))dlsym(dl, fn_symbol);
	if (!fptr) {
		exec_error("dlsym: %s", dlerror());
		return VACCEL_EINVAL;
	}

	ret = (*fptr)(read, nr_read, write, nr_write);
	if (ret)
		return VACCEL_ENOEXEC;

	char *do_dlclose = getenv("VACCEL_EXEC_DLCLOSE");
	if (do_dlclose &&
	    (strcmp(do_dlclose, "1") == 0 || strcmp(do_dlclose, "true") == 0)) {
		if (dlclose(dl)) {
			exec_error("dlclose: %s", dlerror());
			return VACCEL_EINVAL;
		}
	}

	return VACCEL_OK;
}

static int exec_with_resource(struct vaccel_session *session,
			      struct vaccel_resource *resource,
			      const char *fn_symbol, void *read, size_t nr_read,
			      void *write, size_t nr_write)
{
	void *dl;
	void **ddl = NULL;
	int (*fptr)(void *, size_t, void *, size_t);
	int ret;
	struct vaccel_resource **deps;
	size_t nr_deps;
	char *library;
	struct vaccel_arg *args;

	exec_res_debug("Calling exec_with_resource for session %u",
		       session->session_id);

	library = vaccel_resource_get_path(resource);
	if (library == NULL) {
		vaccel_error("Could not get the path of the shared object");
		return VACCEL_EINVAL;
	}
	ret = vaccel_resource_get_deps(&deps, &nr_deps, resource);
	if (ret)
		return VACCEL_EINVAL;

	if (nr_deps) {
		exec_res_debug("nr_deps: %zu", nr_deps);
		ddl = (void **)malloc(sizeof(*ddl) * nr_deps);
		if (!ddl)
			return VACCEL_ENOMEM;
	}
	for (size_t i = 0; i < nr_deps; i++) {
		struct vaccel_resource *res = deps[i];
		const char *fpath = vaccel_resource_get_path(res);

		exec_res_debug("dep library: %s", fpath);
		ddl[i] = dlopen(fpath, RTLD_NOW | RTLD_GLOBAL);
		if (!ddl[i]) {
			exec_res_error("dlopen: %s", dlerror());
			ret = VACCEL_EINVAL;
			goto free;
		}
	}

	exec_res_debug("library: %s", library);
	dl = dlopen(library, RTLD_NOW);
	if (!dl) {
		exec_res_error("dlopen: %s", dlerror());
		ret = VACCEL_EINVAL;
		goto free;
	}

	/* Get the function pointer based on the relevant symbol */
	exec_res_debug("symbol: %s", fn_symbol);
	fptr = (int (*)(void *, size_t, void *, size_t))dlsym(dl, fn_symbol);
	if (!fptr) {
		exec_res_error("%s", dlerror());
		ret = VACCEL_EINVAL;
		goto free;
	}

	args = (struct vaccel_arg *)read;
	for (size_t i = 0; i < nr_read; i++) {
		exec_res_debug("read[%d].size: %u", i, args[i].size);
		exec_res_debug("read[%d].argtype: %u", i, args[i].argtype);
	}
	args = (struct vaccel_arg *)write;
	for (size_t i = 0; i < nr_write; i++) {
		exec_res_debug("write[%d].size: %u", i, args[i].size);
		exec_res_debug("write[%d].argtype: %u", i, args[i].argtype);
	}

	ret = (*fptr)(read, nr_read, write, nr_write);

	char *do_dlclose = getenv("VACCEL_EXEC_DLCLOSE");
	if (do_dlclose &&
	    (strcmp(do_dlclose, "1") == 0 || strcmp(do_dlclose, "true") == 0)) {
		if (dlclose(dl)) {
			exec_res_error("dlclose: %s", dlerror());
			ret = VACCEL_EINVAL;
			goto free;
		}

		for (size_t i = nr_deps; i > 0; i--) {
			if (dlclose(ddl[i - 1])) {
				exec_res_error("dlclose: %s", dlerror());
				ret = VACCEL_EINVAL;
				goto free;
			}
		}
	}

	if (ret)
		ret = VACCEL_ENOEXEC;
	else
		ret = VACCEL_OK;

free:
	free(library);
	if (ddl)
		free(ddl);
	return ret;
}

struct vaccel_op ops[] = {
	VACCEL_OP_INIT(ops[0], VACCEL_NO_OP, noop),
	VACCEL_OP_INIT(ops[1], VACCEL_EXEC, exec),
	VACCEL_OP_INIT(ops[2], VACCEL_EXEC_WITH_RESOURCE, exec_with_resource),
};

static int init(void)
{
	return register_plugin_functions(ops, sizeof(ops) / sizeof(ops[0]));
}

static int fini(void)
{
	return VACCEL_OK;
}

VACCEL_MODULE(.name = "exec", .version = VACCEL_VERSION,
	      .vaccel_version = VACCEL_VERSION,
	      .type = VACCEL_PLUGIN_SOFTWARE | VACCEL_PLUGIN_GENERIC |
		      VACCEL_PLUGIN_CPU,
	      .init = init, .fini = fini)
