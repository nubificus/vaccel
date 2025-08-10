// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "list.h"
#include "resource.h"
#include "session.h"

#ifdef __cplusplus
extern "C" {
#endif

/* A representation of a registration of a resource with a session */
struct resource_registration {
	/* the registered resource */
	struct vaccel_resource *resource;

	/* the session of the registered resource */
	struct vaccel_session *session;

	/* entry for registered resource's sessionsk list */
	struct vaccel_list_entry resource_entry;

	/* entry for session's registered resources list */
	struct vaccel_list_entry session_entry;
};

/* Allocate and initialize resource registration */
int resource_registration_new(struct resource_registration **reg,
			      struct vaccel_resource *res,
			      struct vaccel_session *sess);

/* Release resource registration data and free registration created with
 * `resource_registration_new()` */
int resource_registration_delete(struct resource_registration *reg);

/* Link resource registration to session/resource lists */
int resource_registration_link(struct resource_registration *reg);

/* Unlink resource registration from session/resource lists */
int resource_registration_unlink(struct resource_registration *reg);

/* Find resource registration of resource registered with session */
struct resource_registration *
resource_registration_find(struct vaccel_resource *res,
			   struct vaccel_session *sess);

/* Find resource registration of resource registered with session and unlink
 * from resource/session lists */
struct resource_registration *
resource_registration_find_and_unlink(struct vaccel_resource *res,
				      struct vaccel_session *sess);

/* Iterate resource's list of resource registrations and execute callback
 * function */
int resource_registration_foreach_session(
	struct vaccel_resource *res,
	int (*callback)(struct vaccel_resource *res,
			struct vaccel_session *sess));

/* Iterate session's list of resource registrations and execute callback
 * function */
int resource_registration_foreach_resource(
	struct vaccel_session *sess,
	int (*callback)(struct vaccel_resource *res,
			struct vaccel_session *sess));

#ifdef __cplusplus
}
#endif
