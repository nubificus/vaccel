// SPDX-License-Identifier: Apache-2.0

#include "resources/shared_object.h"
#include "error.h"
#include "log.h"
#include "resources.h"
#include "session.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int shared_object_destructor(void *data)
{
	struct vaccel_shared_object *object =
		(struct vaccel_shared_object *)data;

	if (!object)
		return VACCEL_EINVAL;

	vaccel_file_destroy(&object->file);

	return VACCEL_OK;
}

int vaccel_shared_object_new(struct vaccel_shared_object *object,
			     const char *path)
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

	printf("New file: %s\n", object->file.path);
	return VACCEL_OK;

destroy_file:
	vaccel_file_destroy(&object->file);
free_resource:
	free(res);
	return ret;
}

int vaccel_shared_object_new_with_deps(struct vaccel_shared_object *object,
				       const char *path,
				       const char *dep_paths[], size_t nr_deps)
{
	if (!dep_paths || !nr_deps)
		return VACCEL_EINVAL;

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

	struct vaccel_shared_object *deps = malloc(nr_deps * sizeof(*deps));
	if (!deps)
		goto destroy_file;
	struct vaccel_resource **deps_res = malloc(nr_deps * sizeof(*deps_res));
	if (!deps_res)
		goto free_deps;
	for (size_t i = 0; i < nr_deps; i++) {
		deps_res[i] = NULL;
		deps[i].resource = NULL;
	}
	for (size_t i = 0; i < nr_deps; i++) {
		ret = vaccel_file_new(&deps[i].file, dep_paths[i]);
		if (ret) {
			vaccel_error("file_new: %s", dep_paths[i]);
			ret = VACCEL_ENOMEM;
			goto free_deps_res;
		}

		struct vaccel_resource *dep_res = malloc(sizeof(*res));
		if (!dep_res) {
			ret = VACCEL_ENOMEM;
			goto free_deps_res;
		}
		deps_res[i] = dep_res;

		ret = resource_new(dep_res, VACCEL_RES_SHARED_OBJ,
				   (void *)&deps[i], shared_object_destructor);
		if (ret) {
			vaccel_error("resource_new: %s", dep_paths[i]);
			ret = VACCEL_ENOMEM;
			goto free_deps_res;
		}

		deps[i].resource = deps_res[i];
	}
	ret = resource_set_deps(res, deps_res, nr_deps);
	if (ret)
		goto destroy_file;

	object->resource = res;

	return VACCEL_OK;

free_deps_res:
	for (size_t i = 0; i < nr_deps; i++) {
		if (deps[i].resource) {
			int r = vaccel_shared_object_destroy(&deps[i]);
			if (r)
				vaccel_warn("Could not destroy shared object");
		}
		if (deps_res[i]) {
			free(deps_res[i]);
		}
	}
	free(deps_res);
free_deps:
	free(deps);
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

	int ret = vaccel_file_from_buffer(&object->file, buff, size, NULL, NULL,
					  false, false);
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
const uint8_t *vaccel_shared_object_get(struct vaccel_shared_object *object,
					size_t *len)
{
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

	struct vaccel_resource *resource = object->resource;

	// Do not explicitly cleanup deps created from buffer when on host
	if (!resource->rundir) {
		vaccel_debug("Destroying resource deps: %zu",
			     resource->nr_deps);
		for (size_t i = 0; i < resource->nr_deps; i++) {
			struct vaccel_resource *res = resource->deps[i];
			vaccel_debug("Destroying dep resource %lld", res->id);

			int ret = resource_destroy(res);
			if (ret)
				vaccel_warn("Could not destroy resource");

			free(res);
		}

		free(resource->deps);
	}

	/* This will destroy the underlying resource and call our
	 * destructor callback */
	vaccel_debug("Destroying resource %lld", resource->id);
	int ret = resource_unset_deps(resource);
	if (ret)
		vaccel_warn("Could not unset resource deps");

	ret = resource_destroy(resource);
	if (ret)
		vaccel_warn("Could not destroy resource");

	free(resource);

	return VACCEL_OK;
}

vaccel_id_t
vaccel_shared_object_get_id(const struct vaccel_shared_object *object)
{
	if (!object || !object->resource)
		return -1;

	return object->resource->id;
}

struct vaccel_shared_object *
vaccel_shared_object_from_resource(struct vaccel_resource *resource)
{
	if (!resource)
		return NULL;

	return resource->data;
}

vaccel_id_t
vaccel_shared_object_get_remote_id(const struct vaccel_shared_object *object)
{
	if (!object || !object->resource)
		return -1;

	return object->resource->remote_id;
}
