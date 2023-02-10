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

#include "resources.h"
#include "id_pool.h"
#include "error.h"
#include "list.h"
#include "log.h"
#include "utils.h"
#include "vaccel.h"
#include "plugin.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <sys/stat.h>

#define MAX_RESOURCES 2048
#define MAX_RESOURCE_RUNDIR 1024

static bool initialized = false;
static id_pool_t id_pool;

/* All the live (created) vAccel resources.
 * At the moment, this is an array where each element is a list of all
 * resources of the same time. We should think the data structure again.
 */
static list_t live_resources[VACCEL_RES_MAX];

int resources_bootstrap(void)
{
	int ret = id_pool_new(&id_pool, MAX_RESOURCES);
	if (ret)
		return ret;

	for (int i = 0; i < VACCEL_RES_MAX; ++i)
		list_init(&live_resources[i]);

	initialized = true;

	return VACCEL_OK;
}

int resources_cleanup(void)
{
	if (!initialized)
		return VACCEL_OK;

	for (int i = 0; i < VACCEL_RES_MAX; ++i) {
		struct vaccel_resource *res, *tmp;
		for_each_vaccel_resource_safe(res, tmp, &live_resources[i])
			resource_destroy(res);
	}

	return id_pool_destroy(&id_pool);
}

int resource_new(struct vaccel_resource *res, vaccel_resource_t type,
		void *data, int (*cleanup_resource)(void *))
{
	if (!initialized)
		return VACCEL_EPERM;

	if (!res || type >= VACCEL_RES_MAX)
		return VACCEL_EINVAL;

	/* If we 're working on top of VirtIO, the host side will provide
	 * us with an id */
	struct vaccel_plugin *virtio = get_virtio_plugin(); 
	if (virtio) {
		int err = virtio->info->resource_new(type, data, &res->id);
		if (err)
			return err;
	} else {
		res->id = id_pool_get(&id_pool);
	}

	if (res->id <= 0)
		return VACCEL_EUSERS;

	res->type = type;
	res->data = data;
	res->cleanup_resource = cleanup_resource;
	list_init_entry(&res->entry);
	list_add_tail(&live_resources[0], &res->entry);
	atomic_init(&res->refcount, 0);
	res->rundir = NULL;

	return VACCEL_OK;
}

int resource_get_by_id(struct vaccel_resource **resource, vaccel_id_t id)
{
	if (!initialized)
		return VACCEL_EPERM;

	for (int i = 0; i < VACCEL_RES_MAX; ++i) {
		struct vaccel_resource *res, *tmp;
		for_each_vaccel_resource_safe(res, tmp, &live_resources[i]) {
			if (id == res->id) {
				*resource = res;
				return VACCEL_OK;
			}
		}
	}

	if (*resource != NULL) return VACCEL_OK;
	return VACCEL_EINVAL;
}

int resource_destroy(struct vaccel_resource *res)
{
	if (!initialized)
		return VACCEL_EPERM;

	if (!res)
		return VACCEL_EINVAL;

	/* Check if this resources is currently registered to a session.
	 * We do not destroy currently-used resources */
	if (atomic_load(&res->refcount)) {
		vaccel_warn("Cannot destroy used resource %u", res->id);
		return VACCEL_EBUSY;
	}

	struct vaccel_plugin *virtio = get_virtio_plugin();
	if (virtio) {
		int err = virtio->info->resource_destroy(res->id);
		if (err)
			vaccel_warn("Could not destroy host-side resource %u",
					res->id);
	} else if (res->id) {
		id_pool_release(&id_pool, res->id);
	}

	list_unlink_entry(&res->entry);

	/* Cleanup the type-specific resource */
	if (res->cleanup_resource)
		res->cleanup_resource(res->data);

	if (res->rundir) {
		cleanup_rundir(res->rundir);
		free(res->rundir);
	}

	return VACCEL_OK;
}

void resource_refcount_inc(struct vaccel_resource *res)
{
	if (!res) {
		vaccel_error("BUG! Refcounting invalid resource");
		return;
	}

	atomic_fetch_add(&res->refcount, 1);
}

void resource_refcount_dec(struct vaccel_resource *res)
{
	if (!res) {
		vaccel_error("BUG! Refcounting invalid resource");
		return;
	}

	atomic_fetch_sub(&res->refcount, 1);
}

int resource_create_rundir(struct vaccel_resource *res)
{
	if (!res) {
		vaccel_error("BUG! Trying to create rundir for invalid resource");
		return VACCEL_EINVAL;
	}

	const char *root_rundir = vaccel_rundir();

	char rundir[MAX_RESOURCE_RUNDIR];
	int len = snprintf(rundir, MAX_RESOURCE_RUNDIR, "%s/resource.%lld",
			root_rundir, res->id);
	if (len == MAX_RESOURCE_RUNDIR) {
		vaccel_error("rundir path '%s/resource.%lu' too long",
				root_rundir, res->id);
		return VACCEL_ENAMETOOLONG;
	}

	int ret = mkdir(rundir, 0700);
	if (ret)
		return errno;

	res->rundir = strndup(rundir, MAX_RESOURCE_RUNDIR);
	if (!res->rundir)
		return VACCEL_ENOMEM;

	return VACCEL_OK;
}
