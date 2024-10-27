// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "id.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct session_resources;
struct vaccel_resource;

struct vaccel_session {
	/* id of the session */
	vaccel_id_t id;

	/* session-specific resources */
	struct session_resources *resources;

	/* plugin preference */
	unsigned int hint;

	/* backend private data */
	void *priv;

	/* local or virtio option */
	bool is_virtio;

	/* id of the remote session */
	vaccel_id_t remote_id;
};

/* Initialize a new session with the runtime */
int vaccel_session_init(struct vaccel_session *sess, uint32_t flags);

/* Update a session with new flags */
int vaccel_session_update(struct vaccel_session *sess, uint32_t flags);

/* Tear down a session */
int vaccel_session_free(struct vaccel_session *sess);

/* Check if a resource is registered with a session */
bool vaccel_session_has_resource(const struct vaccel_session *sess,
				 const struct vaccel_resource *res);

/* Deprecated. To be removed. */
int vaccel_sess_init(struct vaccel_session *sess, uint32_t flags);
int vaccel_sess_update(struct vaccel_session *sess, uint32_t flags);
int vaccel_sess_free(struct vaccel_session *sess);
bool vaccel_sess_has_resource(struct vaccel_session *sess,
			      struct vaccel_resource *res);

#ifdef __cplusplus
}
#endif
