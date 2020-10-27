#ifndef __VACCEL_BACKEND_H__
#define __VACCEL_BACKEND_H__

#include <stdint.h>

#include "vaccel_ops.h"
#include "list.h"

#ifdef __cplusplus
extern "C" {
#endif

struct vaccel_backend {
	/* Name of the backend */
	char *name;

	/* Entry for list of backends */
	list_entry_t entry;

	/* List of functions supported by this backend */
	list_t ops;

	/* handle for the dynamic library of the backend */
	void *dl;

	/* backend session init function */
	int (*vaccel_sess_init)(struct vaccel_session *sess, uint32_t flags);

	/* backend session fini function */
	int (*vaccel_sess_free)(struct vaccel_session *sess);

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
int register_virtio_backend(struct vaccel_backend *backend);
int unregister_virtio_backend(struct vaccel_backend *backend);
int register_backend_function(struct vaccel_backend *backend, uint8_t op_type,
		void *func);
void *get_backend_op(uint8_t op_type);
struct vaccel_backend *get_virtio_backend(void);

#ifdef __cplusplus
}
#endif

#endif /* __VACCEL_BACKEND_H__ */
