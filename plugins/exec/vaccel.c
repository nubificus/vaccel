// SPDX-License-Identifier: Apache-2.0

#define _POSIX_C_SOURCE 200809L

#include <byteswap.h>
#include <dlfcn.h>
#include <stdio.h>
#include <vaccel.h>

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

static int exec(struct vaccel_session *session, const char *library,
		const char *fn_symbol, void *read, size_t nr_read, void *write,
		size_t nr_write)
{
	void *dl;
	int (*fptr)(void *, size_t, void *, size_t);
	int ret;
	struct vaccel_arg *args;

	vaccel_debug("Calling exec for session %u", session->session_id);

	vaccel_debug("[exec] library: %s", library);
	dl = dlopen(library, RTLD_NOW);
	if (!dl) {
		vaccel_error("%s", dlerror());
		return VACCEL_EINVAL;
	}

	/* Get the function pointer based on the relevant symbol */
	vaccel_debug("[exec] symbol: %s", fn_symbol);

	args = (struct vaccel_arg *)read;
	for (size_t i = 0; i < nr_read; i++) {
		vaccel_debug("[exec]: read[%d].size: %u\n", i, args[i].size);
		vaccel_debug("[exec]: read[%d].argtype: %u\n", i,
			     args[i].argtype);
	}
	args = (struct vaccel_arg *)write;
	for (size_t i = 0; i < nr_write; i++) {
		vaccel_debug("[exec]: write[%d].size: %u\n", i, args[i].size);
		vaccel_debug("[exec]: write[%d].argtype: %u\n", i,
			     args[i].argtype);
	}

	fptr = (int (*)(void *, size_t, void *, size_t))dlsym(dl, fn_symbol);
	if (!fptr) {
		vaccel_error("%s", dlerror());
		return VACCEL_EINVAL;
	}

	ret = (*fptr)(read, nr_read, write, nr_write);
	if (ret)
		return VACCEL_ENOEXEC;

	return VACCEL_OK;
}

static int exec_with_resource(struct vaccel_session *session,
<<<<<<< HEAD
			      struct vaccel_shared_object *object,
			      const char *fn_symbol, void *read, size_t nr_read,
			      void *write, size_t nr_write)
=======
		struct vaccel_shared_object *object, const char *fn_symbol,
		void *read, size_t nr_read, void *write, size_t nr_write)
>>>>>>> 151ab19 (resources: Enable addition of resource dependencies)
{
	void *dl, **ddl;
	int (*fptr)(void *, size_t, void *, size_t);
	int ret;
	struct vaccel_resource *resource = object->resource;
	struct vaccel_file *file =  &object->file;
	const char *library = file->path;
	struct vaccel_arg *args;

	vaccel_debug("Calling exec_with_resource for session %u",
<<<<<<< HEAD
		     session->session_id);
=======
			session->session_id);
>>>>>>> 151ab19 (resources: Enable addition of resource dependencies)

	if (resource->nr_deps) {
		vaccel_debug("[exec_with_resource] nr_deps: %zu", resource->nr_deps);
		ddl = malloc(sizeof(*ddl) * resource->nr_deps);
		if (!ddl)
			return VACCEL_ENOMEM;
	}
	for (size_t i = 0; i < resource->nr_deps; i++) {
		struct vaccel_resource *res = resource->deps[i];
		struct vaccel_shared_object *object =
			(struct vaccel_shared_object *)res->data;
		const char *fpath = object->file.path;

		vaccel_debug("[exec_with_resource] dep library: %s", fpath);
		ddl[i] = dlopen(fpath, RTLD_NOW|RTLD_GLOBAL);
		if (!ddl[i]) {
			vaccel_error("dlopen: %s", dlerror());
			return VACCEL_EINVAL;
		}
	}

	vaccel_debug("[exec_with_resource] library: %s", library);
	dl = dlopen(library, RTLD_NOW);
	if (!dl) {
		vaccel_error("dlopen: %s", dlerror());
		return VACCEL_EINVAL;
	}

	/* Get the function pointer based on the relevant symbol */
	vaccel_debug("[exec] symbol: %s", fn_symbol);
	fptr = (int (*)(void *, size_t, void *, size_t))dlsym(dl, fn_symbol);
	if (!fptr) {
		vaccel_error("%s", dlerror());
		return VACCEL_EINVAL;
	}

	args = (struct vaccel_arg *)read;
	for (size_t i = 0; i < nr_read; i++) {
		vaccel_debug("[exec]: read[%d].size: %u\n", i, args[i].size);
		vaccel_debug("[exec]: read[%d].argtype: %u\n", i, args[i].argtype);
	}
	args = (struct vaccel_arg *)write;
	for (size_t i = 0; i < nr_write; i++) {
		vaccel_debug("[exec]: write[%d].size: %u\n", i, args[i].size);
		vaccel_debug("[exec]: write[%d].argtype: %u\n", i, args[i].argtype);
	}

	ret = (*fptr)(read, nr_read, write, nr_write);

	// FIXME: Need to dlclose to free the disk resources
	/*
	if (dlclose(dl)) {
		vaccel_error("dlclose: %s", dlerror());
		return VACCEL_EINVAL;
	}

	for (size_t i = resource->nr_deps; i > 0; i--) {
		vaccel_warn("DLClOSE[%d]", i);
		if (dlclose(ddl[i-1])) {
			vaccel_error("dlclose: %s", dlerror());
			return VACCEL_EINVAL;
		}
	}
	*/

	if (ret)
		return VACCEL_ENOEXEC;

	return VACCEL_OK;
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

VACCEL_MODULE(.name = "exec", .version = VACCELRT_VERSION,
	      .type = VACCEL_PLUGIN_SOFTWARE | VACCEL_PLUGIN_GENERIC |
		      VACCEL_PLUGIN_CPU,
	      .init = init, .fini = fini)
