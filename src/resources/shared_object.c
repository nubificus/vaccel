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

#include "resources/shared_object.h"
#include "resources.h"
#include "log.h"
#include "error.h"
#include "session.h"

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

static int shared_object_destructor(void *data)
{
	struct vaccel_shared_object *object = (struct vaccel_shared_object *)data;

	if (!object)
		return VACCEL_EINVAL;

	vaccel_file_destroy(&object->file);

	return VACCEL_OK;
}

int vaccel_shared_object_new(struct vaccel_shared_object *object, const char *path)
{
	struct vaccel_resource *res = malloc(sizeof(*res));
	if (!res)
		return VACCEL_ENOMEM;

	int ret = vaccel_file_new(&object->file, path);
	if (ret)
		goto free_resource;

	ret = resource_new(res, VACCEL_RES_SHARED_OBJ, (void *)object,
			shared_object_destructor);
	if (ret)
		goto destroy_file;

	object->resource = res;

	return VACCEL_OK;

destroy_file:
	vaccel_file_destroy(&object->file);
free_resource:
	free(res);
	return ret;
}

int vaccel_shared_object_new_from_buffer(struct vaccel_shared_object *object,
		const uint8_t *buff, size_t size)
{
	if (!object || !buff || !size)
		return VACCEL_EINVAL;

	struct vaccel_resource *res = malloc(sizeof(*res));
	if (!res)
		return VACCEL_ENOMEM;

	int ret = vaccel_file_from_buffer(&object->file, buff, size, NULL,
			NULL, false, false);
	if (ret)
		goto free_resource;

	ret = resource_new(res, VACCEL_RES_SHARED_OBJ, (void *)object,
			shared_object_destructor);
	if (ret)
		goto destroy_file;

	ret = resource_create_rundir(res);
	if (ret)
		goto destroy_resource;

	vaccel_debug("New rundir for resource %s", res->rundir);

	ret = vaccel_file_persist(&object->file, res->rundir, "lib.so", true);
	if (ret)
		goto destroy_resource;

	object->resource = res;

	return VACCEL_OK;

destroy_resource:
	resource_destroy(res);
destroy_file:
	vaccel_file_destroy(&object->file);
free_resource:
	free(res);

	return ret;
}

/* Get a pointer to the data of the shared object file
 *
 * This will return a pointer to the data of shared object file.
 * If the data have not been read before, this will first try to
 * read the data (mmap them in memory) before returning.
 */
const uint8_t *vaccel_shared_object_get(
	struct vaccel_shared_object *object,
	size_t *len
) {
	if (!object)
		return NULL;

	// vaccel_file_data will read the file in memory if
	// not already done
	return vaccel_file_data(&object->file, len);
}

int vaccel_shared_object_destroy(struct vaccel_shared_object *object)
{
	if (!object)
		return VACCEL_EINVAL;

	vaccel_debug("Destroying resource %u", object->resource->id);
	/* This will destroy the underlying resource and call our
	 * destructor callback */
	int ret = resource_destroy(object->resource);
	if (ret)
		vaccel_warn("Could not destroy resource");

	free(object->resource);

	return VACCEL_OK;
}

vaccel_id_t vaccel_shared_object_get_id(const struct vaccel_shared_object *object)
{
	if (!object || !object->resource)
		return -1;

	return object->resource->id;
}
