/*
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

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
	dl = dlopen(library, RTLD_NOW);
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

static int exec_with_resource(struct vaccel_session *session, struct vaccel_shared_object *object, const char *fn_symbol, void *read, size_t nr_read, void *write,
							  size_t nr_write)
{
	int i = 0;
	void *dl;
	int (*fptr)(void *, size_t, void *, size_t);
	int ret;
	struct vaccel_arg *args;
	struct vaccel_file *file;

	vaccel_debug("Calling exec_with_resource for session %u", session->session_id);

	file = &object->file;
	const char *library = file->path;

	vaccel_debug("[exec_with_resource] library: %s", library);
	dl = dlopen(library, RTLD_NOW);
	if (!dl)
	{
		vaccel_error("%s", dlerror());
		return VACCEL_EINVAL;
	}

	/* Get the function pointer based on the relevant symbol */
	vaccel_debug("[exec] symbol: %s", fn_symbol);
	fptr = (int (*)(void *, size_t, void *, size_t))dlsym(dl, fn_symbol);
	if (!fptr)
	{
		vaccel_error("%s", dlerror());
		return VACCEL_EINVAL;
	}

	ret = (*fptr)(read, nr_read, write, nr_write);
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

VACCEL_MODULE(
		.name = "exec",
		.version = VACCELRT_VERSION,
		.type = VACCEL_PLUGIN_SOFTWARE | VACCEL_PLUGIN_GENERIC | VACCEL_PLUGIN_CPU,
		.init = init,
		.fini = fini)
