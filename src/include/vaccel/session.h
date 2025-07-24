// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "id.h"
#include "resource.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct session_resources;
struct vaccel_plugin;

struct vaccel_session {
	/* id of the session */
	vaccel_id_t id;

	/* id of the remote session */
	vaccel_id_t remote_id;

	/* session-specific resources */
	struct session_resources *resources;

	/* plugin preference */
	unsigned int hint;

	/* backend private data */
	void *priv;

	/* local or virtio option */
	bool is_virtio;

	/* plugin providing the session operations */
	struct vaccel_plugin *plugin;
};

/* Initialize session */
int vaccel_session_init(struct vaccel_session *sess, uint32_t flags);

/* Update session with new flags */
int vaccel_session_update(struct vaccel_session *sess, uint32_t flags);

/* Release session data */
int vaccel_session_release(struct vaccel_session *sess);

/* Allocate and initialize session */
int vaccel_session_new(struct vaccel_session **sess, uint32_t flags);

/* Release session data and free session created with `vaccel_session_new()` */
int vaccel_session_delete(struct vaccel_session *sess);

/* Check if a resource is registered with a session */
bool vaccel_session_has_resource(const struct vaccel_session *sess,
				 const struct vaccel_resource *res);

/* Get resource by id, from registered live resources */
int vaccel_session_resource_by_id(struct vaccel_session *sess,
				  struct vaccel_resource **res, vaccel_id_t id);

/* Get resource by type from live resources.
 * It is required that the resource to be returned, is registered to `sess`
 * session. */
int vaccel_session_resource_by_type(struct vaccel_session *sess,
				    struct vaccel_resource **res,
				    vaccel_resource_type_t type);

/* Get a list of the registered resources, by type */
int vaccel_session_resources_by_type(struct vaccel_session *sess,
				     struct vaccel_resource ***resources,
				     size_t *nr_found,
				     vaccel_resource_type_t type);

/* Deprecated. To be removed. */
int vaccel_sess_init(struct vaccel_session *sess, uint32_t flags);
int vaccel_sess_update(struct vaccel_session *sess, uint32_t flags);
int vaccel_sess_free(struct vaccel_session *sess);
bool vaccel_sess_has_resource(struct vaccel_session *sess,
			      struct vaccel_resource *res);

#ifdef __cplusplus
}
#endif
