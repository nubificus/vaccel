#ifndef __VACCEL_SESSION_H__
#define __VACCEL_SESSION_H__

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

struct session_resources;
struct vaccel_resource;

struct vaccel_session {
	/* id of the session */
	uint32_t session_id;

	/* session-specific resources */
	struct session_resources *resources;

	/* backend private data */
	void *priv;
};

/* Initialize a new session with the runtime */
int vaccel_sess_init(struct vaccel_session *sess, uint32_t flags);

/* Tear down a session */
int vaccel_sess_free(struct vaccel_session *sess);

/* Register a resource with a session */
int vaccel_sess_register(
	struct vaccel_session *sess,
	struct vaccel_resource *resource
);

/* Unregister a resource from a session */
int vaccel_sess_unregister(
	struct vaccel_session *sess,
	struct vaccel_resource *resource
);

/* Check if a resource is registered with a session */
bool vaccel_sess_has_resource(
	struct vaccel_session *sess,
	struct vaccel_resource *resource
);

#ifdef __cplusplus
}
#endif

#endif /* __VACCEL_SESSION_H__ */
