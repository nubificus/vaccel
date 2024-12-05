// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "include/vaccel/session.h" // IWYU pragma: export
#include "list.h"
#include "resource.h"
#include <limits.h>
#include <linux/limits.h>

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
	vaccel_list_t entry;
};

struct session_resources {
	/* Runtime directory for holding resources related with the
	 * runtime */
	char rundir[PATH_MAX];

	/* Resources registered to this session. At the moment, this
	 * is an array where each element holds a list of resources of
	 * the same resource type */
	vaccel_list_t registered[VACCEL_RESOURCE_MAX];
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

/* Helper macros for iterating lists of containers */
#define session_for_each_resource(iter, list)                               \
	list_for_each_container((iter), (list), struct registered_resource, \
				entry)

#define session_for_each_resource_safe(iter, tmp, list)     \
	list_for_each_container_safe((iter), (tmp), (list), \
				     struct registered_resource, entry)

#ifdef __cplusplus
}
#endif
