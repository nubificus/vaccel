// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <stddef.h>
#include <list.h>

#include "error.h"
#include "vaccel_id.h"

#ifdef __cplusplus
#include <atomic>
typedef std::atomic<unsigned int> atomic_uint;
#else
#include <stdatomic.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	VACCEL_RES_SHARED_OBJ = 0,
	VACCEL_RES_SINGLE_MODEL,
	VACCEL_RES_TF_SAVED_MODEL,
	VACCEL_RES_MAX
} vaccel_resource_t;

struct vaccel_resource {
	/* resource id */
	vaccel_id_t id;

	/* type of the resource */
	vaccel_resource_t type;

	/* type-specific data of the resource */
	void *data;

	/* type-specific destructor */
	int (*cleanup_resource)(void *data);

	/* An entry to add this resource in a list */
	list_entry_t entry;

	/* Reference counter representing the number of sessions
	 * to which this resource is registered to. */
	atomic_uint refcount;

	/* rundir for this resource if it needs it. It can be empty (NULL) */
	char *rundir;

	struct vaccel_resource **deps;

	size_t nr_deps;

	/* resource id for the agent */
	vaccel_id_t remote_id;
};



int vaccel_resource_get_deps(struct vaccel_resource ***deps, size_t *nr_deps,
			     struct vaccel_resource *res);
int vaccel_resource_deps_to_ids(vaccel_id_t *ids, struct vaccel_resource **deps,
				size_t nr_deps);
int vaccel_resource_deps_from_ids(struct vaccel_resource **deps,
				  vaccel_id_t *ids, size_t nr_ids);
int vaccel_resource_set_deps_from_ids(struct vaccel_resource *res,
				      vaccel_id_t *ids, size_t nr_ids);

int resource_get_by_id(struct vaccel_resource **resource, vaccel_id_t id);
#ifdef __cplusplus
}
#endif
