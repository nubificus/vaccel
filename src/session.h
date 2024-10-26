// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "include/session.h"
#include "list.h"
#include "resource.h"
#include <limits.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Struct used to represent a resource registered
 * to a session
 */
struct registered_resource {
	/* The actual resource */
	struct vaccel_resource *res;

	/* List entry to link the resource to the
	 * session's registered resources */
	list_t entry;
};

#define for_each_session_resource(iter, list) \
	for_each_container((iter), (list), struct registered_resource, entry)

#define for_each_session_resource_safe(iter, tmp, list) \
	for_each_container_safe((iter), (tmp), (list),  \
				struct registered_resource, entry)

struct session_resources {
	/* Runtime directory for holding resources related with the
	 * runtime */
	char rundir[PATH_MAX];

	/* Resources registered to this session. At the moment, this
	 * is an array where each element holds a list of resources of
	 * the same resource type */
	list_t registered[VACCEL_RESOURCE_MAX];
};

/* Initialize shared session objects */
int sessions_bootstrap(void);

/* Cleanup shared session objects */
int sessions_cleanup(void);

/* Register a resource with a session */
int session_register_resource(struct vaccel_session *sess,
			      struct vaccel_resource *res);

/* Unregister a resource from a session */
int session_unregister_resource(struct vaccel_session *sess,
				struct vaccel_resource *res);

#ifdef __cplusplus
}
#endif
