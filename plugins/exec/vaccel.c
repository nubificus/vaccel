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

#define exec_warn(fmt, ...) vaccel_warn("[exec] " fmt, ##__VA_ARGS__)
#define exec_debug(fmt, ...) vaccel_debug("[exec] " fmt, ##__VA_ARGS__)
#define exec_error(fmt, ...) vaccel_error("[exec] " fmt, ##__VA_ARGS__)

static int get_dlopen_mode(int default_value)
{
	const char *mode_env = getenv(DLOPEN_MODE_ENV);
	if (!mode_env)
		return default_value;

	if (strcasecmp(mode_env, "lazy") == 0)
		return RTLD_LAZY;
	if (strcasecmp(mode_env, "now") == 0)
		return RTLD_NOW;

	exec_warn("Invalid value '%s' for %s. Using default: %s", mode_env,
		  DLOPEN_MODE_ENV, default_value == RTLD_NOW ? "now" : "lazy");
	return default_value;
}

static bool get_dlclose_enabled(bool default_value)
{
	const char *close_env = getenv(DLCLOSE_ENABLED_ENV);
	const char *close_old_env = getenv(DLCLOSE_ENABLED_OLD_ENV);
	const char *dlclose_enabled_env = close_env;

	if (close_old_env && !close_env) {
		exec_warn("%s is deprecated. Use %s instead.\n",
			  DLCLOSE_ENABLED_OLD_ENV, DLCLOSE_ENABLED_ENV);
		dlclose_enabled_env = close_old_env;
	}

	if (!dlclose_enabled_env)
		return default_value;

	if (strcmp(dlclose_enabled_env, "0") == 0 ||
	    strcasecmp(dlclose_enabled_env, "false") == 0)
		return false;

	if (strcmp(dlclose_enabled_env, "1") == 0 ||
	    strcasecmp(dlclose_enabled_env, "true") == 0)
		return true;

	exec_warn("Invalid value '%s' for %s. Using default: %s",
		  dlclose_enabled_env, DLCLOSE_ENABLED_ENV,
		  default_value ? "true" : "false");
	return default_value;
}

static int noop(struct vaccel_session *session)
{
	exec_debug("session:%" PRId64 " Calling no-op", session->id);
	exec_debug("[noop]");

	return VACCEL_OK;
}

typedef int (*unpack_fn_t)(struct vaccel_arg *read, size_t nr_read,
			   struct vaccel_arg *write, size_t nr_write);

static int exec(struct vaccel_session *session, const char *library,
		const char *fn_symbol, void *read, size_t nr_read, void *write,
		size_t nr_write)
{
	exec_debug("session:%" PRId64 " Calling exec", session->id);

	/* Load main library */
	exec_debug("Library: %s", library);
	void *dl = dlopen(library, get_dlopen_mode(RTLD_NOW));
	if (!dl) {
		exec_error("dlopen failed: %s", dlerror());
		return VACCEL_EINVAL;
	}

	/* Get the function pointer for the specified symbol */
	exec_debug("Symbol: %s", fn_symbol);
	unpack_fn_t unpack = dlsym(dl, fn_symbol);
	if (!unpack) {
		exec_error("dlsym failed: %s", dlerror());
		return VACCEL_ENOSYS;
	}

	char type_name[VACCEL_ENUM_STR_MAX];
	struct vaccel_arg *args = (struct vaccel_arg *)read;
	for (size_t i = 0; i < nr_read; i++) {
		exec_debug("read[%zu].size: %zu", i, args[i].size);
		vaccel_arg_type_name(args[i].type, type_name,
				     VACCEL_ENUM_STR_MAX);
		exec_debug("read[%zu].type: %s", i, type_name);
	}
	args = (struct vaccel_arg *)write;
	for (size_t i = 0; i < nr_write; i++) {
		exec_debug("write[%zu].size: %zu", i, args[i].size);
		vaccel_arg_type_name(args[i].type, type_name,
				     VACCEL_ENUM_STR_MAX);
		exec_debug("write[%zu].type: %s", i, type_name);
	}

	/* Execute the operation */
	int ret = unpack(read, nr_read, write, nr_write);
	if (ret)
		return VACCEL_EBACKEND;

	/* Unload libraries if enabled (disabled by default) */
	if (get_dlclose_enabled(false)) {
		if (dlclose(dl)) {
			exec_error("dlclose failed: %s", dlerror());
			return VACCEL_EINVAL;
		}
	}

	return VACCEL_OK;
}

static void *dlopen_with_fallback(const char *library, int flags)
{
	int primary_mode = (flags & RTLD_LAZY) ? RTLD_LAZY : RTLD_NOW;
	int fallback_mode = (primary_mode == RTLD_LAZY) ? RTLD_NOW : RTLD_LAZY;
	int other_flags = flags & ~(RTLD_NOW | RTLD_LAZY);

	void *handle = dlopen(library, primary_mode | other_flags);
	if (handle)
		return handle;

	exec_warn("dlopen with %s failed for %s: %s",
		  (primary_mode == RTLD_LAZY) ? "RTLD_LAZY" : "RTLD_NOW",
		  library, dlerror());
	exec_warn("Trying %s fallback",
		  (fallback_mode == RTLD_LAZY) ? "RTLD_LAZY" : "RTLD_NOW");

	return dlopen(library, fallback_mode | other_flags);
}

static int load_resource_libs(struct vaccel_resource *res, void **main_handle)
{
	if (!res || !main_handle)
		return VACCEL_EINVAL;

	void **dl = NULL;
	void *main_dl = NULL;
	size_t nr_deps = res->nr_blobs - 1;
	char *library = res->blobs[nr_deps]->path;
	if (res->plugin_priv) {
		/* If libraries are already loaded just return the handle */
		dl = (void **)res->plugin_priv;
		main_dl = dl[nr_deps];
		if (!main_dl) {
			exec_error("Invalid dlopen handle for %s", library);
			return VACCEL_EINVAL;
		}

		*main_handle = main_dl;
		return VACCEL_OK;
	}

	/* Allocate array for dl handles */
	dl = (void **)malloc(sizeof(*dl) * res->nr_blobs);
	if (!dl)
		return VACCEL_ENOMEM;

	/* Load dependency libraries if any */
	int dlopen_mode = get_dlopen_mode(RTLD_NOW);
	for (size_t i = 0; i < nr_deps; i++) {
		char *dep_library = res->blobs[i]->path;
		dl[i] = dlopen_with_fallback(dep_library,
					     dlopen_mode | RTLD_GLOBAL);
		if (!dl[i]) {
			exec_error("dlopen failed for %s: %s", dep_library,
				   dlerror());
			free(dl);
			return VACCEL_EINVAL;
		}
	}

	/* Load main library */
	exec_debug("Library: %s", library);
	if (nr_deps)
		dl[nr_deps] = dlopen_with_fallback(library, dlopen_mode);
	else
		dl[nr_deps] = dlopen(library, dlopen_mode);

	main_dl = dl[nr_deps];
	if (!main_dl) {
		exec_error("dlopen failed: %s", library, dlerror());
		free(dl);
		return VACCEL_EINVAL;
	}

	res->plugin_priv = (void *)dl;
	*main_handle = main_dl;
	return VACCEL_OK;
}

static int exec_with_resource(struct vaccel_session *session,
			      struct vaccel_resource *resource,
			      const char *fn_symbol, void *read, size_t nr_read,
			      void *write, size_t nr_write)
{
	int ret;

	exec_debug("session:%" PRId64 " Calling exec_with_resource",
		   session->id);

	if (resource->nr_blobs < 1) {
		exec_error("No library provided");
		return VACCEL_EINVAL;
	}
	exec_debug("Number of libraries: %zu", resource->nr_blobs);

	void *dl = NULL;
	ret = load_resource_libs(resource, &dl);
	if (ret) {
		exec_error("Failed to load resource libraries");
		return ret;
	}

	/* Get the function pointer for the specified symbol */
	exec_debug("Symbol: %s", fn_symbol);
	unpack_fn_t unpack = dlsym(dl, fn_symbol);
	if (!unpack) {
		exec_error("dlsym failed: %s", dlerror());
		return VACCEL_ENOSYS;
	}

	char type_name[VACCEL_ENUM_STR_MAX];
	struct vaccel_arg *args = (struct vaccel_arg *)read;
	for (size_t i = 0; i < nr_read; i++) {
		exec_debug("read[%zu].size: %zu", i, args[i].size);
		vaccel_arg_type_name(args[i].type, type_name,
				     VACCEL_ENUM_STR_MAX);
		exec_debug("read[%zu].type: %s", i, type_name);
	}
	args = (struct vaccel_arg *)write;
	for (size_t i = 0; i < nr_write; i++) {
		exec_debug("write[%zu].size: %zu", i, args[i].size);
		vaccel_arg_type_name(args[i].type, type_name,
				     VACCEL_ENUM_STR_MAX);
		exec_debug("write[%zu].type: %s", i, type_name);
	}

	/* Execute the operation */
	ret = unpack(read, nr_read, write, nr_write);
	return !ret ? VACCEL_OK : VACCEL_EBACKEND;
}

static int resource_unregister(struct vaccel_resource *res,
			       struct vaccel_session *sess)
{
	(void)sess;

	if (res->type != VACCEL_RESOURCE_LIB || !res->plugin_priv ||
	    !get_dlclose_enabled(true))
		return VACCEL_OK;

	/* Unload libraries if enabled (enabled by default) */
	int ret = VACCEL_OK;
	void **dl = (void **)res->plugin_priv;
	for (size_t i = res->nr_blobs; i > 0; i--) {
		char *library = res->blobs[i - 1]->path;
		if (dlclose(dl[i - 1])) {
			exec_error("dlclose failed for %s: %s", library,
				   dlerror());
			ret = VACCEL_EINVAL;
			goto free;
		}
	}

free:
	free(dl);
	res->plugin_priv = NULL;
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
	      .init = init, .fini = fini,
	      .resource_unregister = resource_unregister)
