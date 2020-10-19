#ifndef __VACCEL_BACKEND_H__
#define __VACCEL_BACKEND_H__

#include <stdint.h>

#include "vaccel_ops.h"
#include "list.h"

struct vaccel_backend {
	/* Name of the backend */
	char *name;

	/* Entry for list of backends */
	list_entry_t entry;

	/* List of functions supported by this backend */
	list_t ops;

	/* handle for the dynamic library of the backend */
	void *dl;

	/* function to call for cleaning up resources allocated
	 * by the backend
	 */
	int (*fini)(struct vaccel_backend *);
};

int backends_bootstrap();
int initialize_backend(struct vaccel_backend *backend, const char *name);
int cleanup_backend(struct vaccel_backend *backend);
int register_backend(struct vaccel_backend *backend);
int unregister_backend(struct vaccel_backend *backend);
int cleanup_backends(void);
int register_backend_function(struct vaccel_backend *backend, uint8_t op_type,
		void *func);
void *get_backend_op(uint8_t op_type);

#endif /* __VACCEL_BACKEND_H__ */
