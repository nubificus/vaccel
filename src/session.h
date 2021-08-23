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

#ifndef __SESSION_H__
#define __SESSION_H__

#include "include/session.h"

#include "resources.h"
#include "list.h"

#define MAX_SESSION_RUNDIR_PATH 512

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

#define for_each_session_resource(iter, list)                                 \
	for_each_container((iter), (list), struct registered_resource, entry)

#define for_each_session_resource_safe(iter, tmp, list)                       \
	for_each_container_safe((iter), (tmp), (list),		              \
			struct registered_resource, entry)

struct session_resources {
	/* Runtime directory for holding resources related with the
	 * runtime */
	char rundir[MAX_SESSION_RUNDIR_PATH];

	/* Resources registered to this session. At the moment, this
	 * is an array where each element holds a list of resources of
	 * the same resource type */
	list_t registered[VACCEL_RES_MAX];
};

int sessions_bootstrap(void);
int sessions_cleanup(void);

#endif /* __VACCEL_SESSION_H__ */
