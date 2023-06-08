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

	/* plugin preference */
	unsigned int hint;

	/* backend private data */
	void *priv;
};

/* Initialize a new session with the runtime */
int vaccel_sess_init(struct vaccel_session *sess, uint32_t flags);

/* Update a session with new flags */
int vaccel_sess_update(struct vaccel_session *sess, uint32_t flags);

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
