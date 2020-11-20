#include <assert.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <dlfcn.h>

#include "backend.h"
#include "common.h"
#include "list.h"
#include "vaccel_ops.h"

struct vaccel_op {
	/* operation type */
	uint8_t type;

	/* function implementing the operation */
	void *func;

	/* backend to which this implementation belongs */
	struct vaccel_backend *owner;

	/* Entry for list of backend functions */
	list_entry_t backend_entry;

	/* Entry for global list of functions of this type */
	list_entry_t func_entry;
};

static struct {
	/* true if sub-system is initialized */
	bool initialized;

	/* list holding backend descriptors */
	list_t backends;

	/* virtio backend */
	struct vaccel_backend *virtio;

	/* array of available implementations for every supported
	 * function
	 */
	list_t ops[VACCEL_FUNCTIONS_NR];
} backend_state = {0};

int backends_bootstrap()
{
	list_init(&backend_state.backends);

	for (size_t i = 0; i < VACCEL_FUNCTIONS_NR; ++i)
		list_init(&backend_state.ops[i]);

	backend_state.initialized = true;

	return VACCEL_OK;
}

int initialize_backend(struct vaccel_backend *backend, const char *name)
{
	if (!backend)
		return VACCEL_EINVAL;

	if (!name)
		return VACCEL_EINVAL;

	if (!backend_state.initialized)
		return VACCEL_EBACKEND;

	backend->name = strdup(name);
	if (!backend->name)
		return VACCEL_ENOMEM;

	list_init_entry(&backend->entry);
	list_init(&backend->ops);

	return VACCEL_OK;
}

int cleanup_backend(struct vaccel_backend *backend)
{
	if (!backend)
		return VACCEL_EINVAL;

	if (!backend_state.initialized)
		return VACCEL_EBACKEND;

	/* name should always be non-NULL */
	if (backend->name) {
		assert(0 && "Backend name not set");
		free(backend->name);
	}

	/* Throw an error if the backend is still registered
	 * or it still has functions registered with it */
	if (entry_linked(&backend->entry)) {
		assert(0 && "Trying to cleanup a registered backend");
		return VACCEL_EBUSY;
	}

	if (!list_empty(&backend->ops)) {
		assert(0 && "Backend has registered functions during cleanup");
		return VACCEL_EBUSY;
	}

	return VACCEL_OK;
}

int register_backend(struct vaccel_backend *backend)
{
	if (!backend)
		return VACCEL_EINVAL;

	if (!backend_state.initialized)
		return VACCEL_EBACKEND;

	if (entry_linked(&backend->entry))
		return VACCEL_EEXISTS;

	if (!list_empty(&backend->ops))
		return VACCEL_EINVAL;

	list_add_tail(&backend_state.backends, &backend->entry);

	return VACCEL_OK;
}

int unregister_backend(struct vaccel_backend *backend)
{
	if (!backend)
		return VACCEL_EINVAL;

	if (!backend_state.initialized)
		return VACCEL_EBACKEND;

	if (!backend)
		return VACCEL_EINVAL;

	if (!entry_linked(&backend->entry)) {
		assert(0 && "Trying to unregister unknown backend");
		return VACCEL_ENOENT;
	}

	/* unregister backend's functions */
	struct vaccel_op *op, *tmp;
	for_each_container_safe(op, tmp, &backend->ops, struct vaccel_op, backend_entry) {
		list_unlink_entry(&op->func_entry);
		list_unlink_entry(&op->backend_entry);
		free(op);
	}

	list_unlink_entry(&backend->entry);

	return VACCEL_OK;
}

int cleanup_backends(void)
{
	if (!backend_state.initialized)
		return VACCEL_OK;

	struct vaccel_backend *backend, *tmp;
	for_each_container_safe(backend, tmp, &backend_state.backends, struct vaccel_backend, entry) {
		/* Unregister backend from runtime */
		unregister_backend(backend);

		/* Clean-up plugin's resources */
		backend->fini(backend);
		dlclose(backend->dl);
		free(backend);
	}

	if (backend_state.virtio)
		backend_state.virtio = NULL;

	backend_state.initialized = false;

	return VACCEL_OK;
}

int register_virtio_backend(struct vaccel_backend *backend)
{
	if (!backend)
		return VACCEL_EINVAL;

	if (!backend->vaccel_sess_init || !backend->vaccel_sess_free)
		return VACCEL_EINVAL;

	if (backend_state.virtio)
		return VACCEL_EEXISTS;

	int ret = register_backend(backend);
	if (ret)
		return ret;

	backend_state.virtio = backend;
	return VACCEL_OK;
}

int unregister_virtio_backend(struct vaccel_backend *backend)
{
	if (!backend)
		return VACCEL_EINVAL;

	if (!backend_state.virtio)
		return VACCEL_ENOENT;

	if (backend_state.virtio != backend)
		return VACCEL_EINVAL;

	int ret = unregister_backend(backend);
	if (ret)
		return ret;

	backend_state.virtio = NULL;
	return VACCEL_OK;
}

int register_backend_function(struct vaccel_backend *backend, uint8_t op_type,
		void *func)
{
	if (!backend || !func || (op_type >= VACCEL_FUNCTIONS_NR))
		return VACCEL_EINVAL;

	struct vaccel_op *new_op = malloc(sizeof(*new_op));
	if (!new_op)
		return VACCEL_ENOMEM;

	new_op->type = op_type;
	new_op->func = func;
	new_op->owner = backend;

	list_add_tail(&backend->ops, &new_op->backend_entry);
	list_add_tail(&backend_state.ops[op_type], &new_op->func_entry);

	return VACCEL_OK;
}

void *get_backend_op(uint8_t op_type)
{
	if (op_type >= VACCEL_FUNCTIONS_NR)
		return NULL;

	if (list_empty(&backend_state.ops[op_type]))
		return NULL;

	/* At the moment, just return the first implementation we find */
	struct vaccel_op *op =
		get_container(backend_state.ops[op_type].next,
				struct vaccel_op, func_entry);

	return op->func;
}

struct vaccel_backend *get_virtio_backend(void)
{
	return backend_state.virtio;
}
