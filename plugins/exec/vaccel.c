// SPDX-License-Identifier: Apache-2.0

#define _POSIX_C_SOURCE 200809L

#include "vaccel.h"
#include <dlfcn.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#define DLOPEN_MODE_ENV "VACCEL_EXEC_DLOPEN_MODE"
#define DLCLOSE_ENABLED_ENV "VACCEL_EXEC_DLCLOSE_ENABLED"
#define DLCLOSE_ENABLED_OLD_ENV "VACCEL_EXEC_DLCLOSE"

#define exec_debug(fmt, ...) vaccel_debug("[exec] " fmt, ##__VA_ARGS__)
#define exec_error(fmt, ...) vaccel_error("[exec] " fmt, ##__VA_ARGS__)

#define exec_res_debug(fmt, ...) \
	vaccel_debug("[exec_with_resource] " fmt, ##__VA_ARGS__)
#define exec_res_error(fmt, ...) \
	vaccel_error("[exec_with_resource] " fmt, ##__VA_ARGS__)

static int get_dlopen_mode(void)
{
	const char *mode_env = getenv(DLOPEN_MODE_ENV);

	if (mode_env && strcasecmp(mode_env, "lazy") == 0)
		return RTLD_LAZY;
	return RTLD_NOW;
}

static bool get_dlclose_enabled(void)
{
	const char *close_env = getenv(DLCLOSE_ENABLED_ENV);
	const char *close_old_env = getenv(DLCLOSE_ENABLED_OLD_ENV);
	const char *dlclose_enabled_env = close_env;

	if (close_old_env && !close_env) {
		vaccel_warn("%s is deprecated. Use %s instead.\n",
			    DLCLOSE_ENABLED_OLD_ENV, DLCLOSE_ENABLED_ENV);
		dlclose_enabled_env = close_old_env;
	}

	if (dlclose_enabled_env &&
	    (strcmp(dlclose_enabled_env, "1") == 0 ||
	     strcasecmp(dlclose_enabled_env, "true") == 0))
		return true;
	return false;
}

static int noop(struct vaccel_session *session)
{
	exec_debug("session:%" PRId64 " Calling no-op", session->id);
	exec_debug("[noop]");

	return VACCEL_OK;
}

static int exec(struct vaccel_session *session, const char *library,
		const char *fn_symbol, void *read, size_t nr_read, void *write,
		size_t nr_write)
{
	exec_debug("session:%" PRId64 " Calling exec", session->id);

	/* Load main library */
	exec_debug("Library: %s", library);
	void *dl = dlopen(library, get_dlopen_mode());
	if (!dl) {
		exec_error("dlopen: %s", dlerror());
		return VACCEL_EINVAL;
	}

	struct vaccel_arg *args = (struct vaccel_arg *)read;
	for (size_t i = 0; i < nr_read; i++) {
		exec_debug("read[%d].size: %u", i, args[i].size);
		exec_debug("read[%d].argtype: %u", i, args[i].argtype);
	}
	args = (struct vaccel_arg *)write;
	for (size_t i = 0; i < nr_write; i++) {
		exec_debug("write[%d].size: %u", i, args[i].size);
		exec_debug("write[%d].argtype: %u", i, args[i].argtype);
	}

	/* Get the function pointer based on the relevant symbol */
	exec_debug("Symbol: %s", fn_symbol);
	int (*fptr)(void *, size_t, void *, size_t) =
		(int (*)(void *, size_t, void *, size_t))dlsym(dl, fn_symbol);
	if (!fptr) {
		exec_error("dlsym: %s", dlerror());
		return VACCEL_EINVAL;
	}

	/* Execute the operation */
	int ret = (*fptr)(read, nr_read, write, nr_write);
	if (ret)
		return VACCEL_ENOEXEC;

	/* Unload libraries if chosen */
	if (get_dlclose_enabled()) {
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
	int ret;

	exec_res_debug("session:%" PRId64 " Calling exec_with_resource",
		       session->id);

	if (resource->nr_files < 1) {
		exec_res_error("No library provided");
		return VACCEL_EINVAL;
	}
	exec_res_debug("Number of libraries: %zu", resource->nr_files);

	/* Allocate array for dl handles */
	void **dl = (void **)malloc(sizeof(*dl) * resource->nr_files);
	if (!dl)
		return VACCEL_ENOMEM;

	/* Load dependency libraries if any */
	int dlopen_mode = get_dlopen_mode();
	size_t nr_deps = resource->nr_files - 1;
	for (size_t i = 0; i < nr_deps; i++) {
		char *dep_library = resource->files[i]->path;
		dl[i] = dlopen(dep_library, dlopen_mode | RTLD_GLOBAL);
		if (!dl[i]) {
			exec_res_error("dlopen %s: %s", dep_library, dlerror());
			ret = VACCEL_EINVAL;
			goto free;
		}
	}

	/* Load main library */
	char *library = resource->files[nr_deps]->path;
	exec_res_debug("Library: %s", library);
	dl[nr_deps] = dlopen(library, dlopen_mode);
	void *ldl = dl[nr_deps];
	if (!ldl) {
		exec_res_error("dlopen %s: %s", library, dlerror());
		ret = VACCEL_EINVAL;
		goto free;
	}

	/* Get the function pointer based on the relevant symbol */
	exec_res_debug("Symbol: %s", fn_symbol);
	int (*fptr)(void *, size_t, void *, size_t) =
		(int (*)(void *, size_t, void *, size_t))dlsym(ldl, fn_symbol);
	if (!fptr) {
		exec_res_error("dlsym: %s", dlerror());
		ret = VACCEL_EINVAL;
		goto free;
	}

	struct vaccel_arg *args = (struct vaccel_arg *)read;
	for (size_t i = 0; i < nr_read; i++) {
		exec_res_debug("read[%d].size: %u", i, args[i].size);
		exec_res_debug("read[%d].argtype: %u", i, args[i].argtype);
	}
	args = (struct vaccel_arg *)write;
	for (size_t i = 0; i < nr_write; i++) {
		exec_res_debug("write[%d].size: %u", i, args[i].size);
		exec_res_debug("write[%d].argtype: %u", i, args[i].argtype);
	}

	/* Execute the operation */
	ret = (*fptr)(read, nr_read, write, nr_write);

	/* Unload libraries if chosen */
	if (get_dlclose_enabled()) {
		for (size_t i = resource->nr_files; i > 0; i--) {
			char *dep_library = resource->files[i - 1]->path;
			if (dlclose(dl[i - 1])) {
				exec_res_error("dlclose %s: %s", dep_library,
					       dlerror());
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
	free(dl);

	return ret;
}

struct vaccel_op ops[] = {
	VACCEL_OP_INIT(ops[0], VACCEL_OP_NOOP, noop),
	VACCEL_OP_INIT(ops[1], VACCEL_OP_EXEC, exec),
	VACCEL_OP_INIT(ops[2], VACCEL_OP_EXEC_WITH_RESOURCE,
		       exec_with_resource),
};

static int init(void)
{
	return vaccel_plugin_register_ops(ops, sizeof(ops) / sizeof(ops[0]));
}

static int fini(void)
{
	return VACCEL_OK;
}

VACCEL_PLUGIN(.name = "exec", .version = VACCEL_VERSION,
	      .vaccel_version = VACCEL_VERSION,
	      .type = VACCEL_PLUGIN_SOFTWARE | VACCEL_PLUGIN_GENERIC |
		      VACCEL_PLUGIN_CPU,
	      .init = init, .fini = fini)
