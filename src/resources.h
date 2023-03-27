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

#ifndef __RESOURCES_H__
#define __RESOURCES_H__

#include <stdint.h>
#include <stdatomic.h>

#include "include/resources.h"
#include "id_pool.h"
#include "list.h"

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
};

int resources_bootstrap(void);
int resources_cleanup(void);
int resource_new(struct vaccel_resource *res, vaccel_resource_t type, void *data,
		int (*cleanup_resource)(void *));
int resource_get_by_id (struct vaccel_resource **resource, vaccel_id_t id);
int resource_destroy(struct vaccel_resource *res);
void resource_refcount_inc(struct vaccel_resource *res);
void resource_refcount_dec(struct vaccel_resource *res);
int resource_create_rundir(struct vaccel_resource *res);
int resource_destroy_rundir(struct vaccel_resource *res);

/* Helper macros for iterating lists of containers */ 
#define for_each_vaccel_resource(iter, list)                                  \
	for_each_container((iter), (list), struct vaccel_resource, entry)

#define for_each_vaccel_resource_safe(iter, tmp, list)                        \
	for_each_container_safe((iter), (tmp), (list), struct vaccel_resource,\
			entry)

#endif /* __RESOURCES_H__ */
