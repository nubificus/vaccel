// SPDX-License-Identifier: Apache-2.0

#define _POSIX_C_SOURCE 200809L

#include "resources/single_model.h"
#include "error.h"
#include "log.h"
#include "resources.h"
#include "session.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int single_model_destructor(void *data)
{
	struct vaccel_single_model *model = (struct vaccel_single_model *)data;

	if (!model)
		return VACCEL_EINVAL;

	vaccel_file_destroy(&model->file);

	return VACCEL_OK;
}

struct vaccel_single_model *vaccel_single_model_new(void)
{
	struct vaccel_single_model *new = calloc(1, sizeof(*new));
	if (!new)
		vaccel_error("Could not allocate memory");

	return new;
}

/* Set the export directory of a model
 *
 * Used to create a model from an export path. The call
 * will check for the existence of the file but will not
 * check for its validity.
 *
 * If the exported model is malformed this will not fail. We will
 * find that out when trying to actually load the model.
 */
int vaccel_single_model_set_path(struct vaccel_single_model *model,
				 const char *path)
{
	if (!model)
		return VACCEL_EINVAL;

	/* Set the model vaccel_file */
	int ret = vaccel_file_new(&model->file, path);
	if (ret) {
		vaccel_error("Could not find model file");
		return VACCEL_ENOENT;
	}

	model->path = strdup(path);
	if (!model->path) {
		ret = VACCEL_ENOMEM;
		goto destroy_model_file;
	}

	model->resource = NULL;
	vaccel_debug("Set single model path to %s", model->path);

	return VACCEL_OK;

destroy_model_file:
	vaccel_file_destroy(&model->file);

	return ret;
}

/* Get the path of the model
 *
 * This will be NULL if the path is not owned
 */
const char *vaccel_single_model_get_path(struct vaccel_single_model *model)
{
	if (!model)
		return NULL;

	return model->path;
}

/* Set the model file from in-memory data
 *
 * Create the underlying file resource for the model file.
 * This will take ownership of the buffer passed as an argument.
 */
int vaccel_single_model_set_file(struct vaccel_single_model *model,
				 const char *filename, const uint8_t *ptr,
				 size_t len)
{
	vaccel_debug("Setting file resource for single model");

	if (!model) {
		vaccel_error("Invalid model");
		return VACCEL_EINVAL;
	}

	if (!ptr || !len) {
		vaccel_error("Invalid data for model");
		return VACCEL_EINVAL;
	}

	if (filename)
		model->filename = strdup(filename);
	else
		model->filename = NULL;

	int ret = vaccel_file_from_buffer(&model->file, ptr, len, NULL, false,
					  NULL, false);
	return ret;
}

/* Get a pointer to the data of the model
 *
 * This will return a pointer to the data of the model file.
 * If the data have not been read before, this will first try to
 * read the data (mmap them in memory) before returning.
 */
const uint8_t *vaccel_single_model_get_file(struct vaccel_single_model *model,
					    size_t *len)
{
	if (!model)
		return NULL;

	// vaccel_file_data will read the file in memory if
	// not already done
	return vaccel_file_data(&model->file, len);
}

/* Register the file as a vAccel resource
 *
 * If the model is created from in-memory data, this will first
 * create the model export directory and persist the file before
 * creating the resource. If no filename is supplied when persisting,
 * 'model.XXXXXX' (where X is random) will be used instead.
 *
 *  This function should be used in conjunction with either
 *  vaccel_single_model_set_path() or vaccel_single_model_set_file().
 */
int vaccel_single_model_register(struct vaccel_single_model *model)
{
	vaccel_debug("Registering new vAccel single model");

	if (!vaccel_file_initialized(&model->file)) {
		vaccel_error("Will not register uninitialized resource");
		return VACCEL_EINVAL;
	}

	struct vaccel_resource *res = malloc(sizeof(*model->resource));
	if (!res) {
		vaccel_error("Could not allocate memory");
		return VACCEL_ENOMEM;
	}

	int ret = resource_new(res, VACCEL_RES_SINGLE_MODEL, (void *)model,
			       single_model_destructor);
	if (ret)
		goto free_resource;

	/* If we have created the model from a path in the disk we are
	 * done here */
	if (model->path)
		goto out;

	/* Else we need to persist the file in the disk */
	ret = resource_create_rundir(res);
	if (ret)
		goto destroy_resource;

	bool random = false;
	if (!model->filename) {
		model->filename = strdup("model");
		random = true;
	}

	ret = vaccel_file_persist(&model->file, res->rundir, model->filename,
				  random);
	if (ret)
		goto destroy_resource;

	model->path = res->rundir;
out:
	vaccel_debug("New resource %lld", res->id);
	model->resource = res;

	return VACCEL_OK;

destroy_resource:
	resource_destroy(res);
free_resource:
	free(res);

	return ret;
}

/* Destroy the model resource
 *
 * This will handle the destruction of the underlying resource and
 * release any resources allocated for the model.
 */
int vaccel_single_model_destroy(struct vaccel_single_model *model)
{
	if (!model)
		return VACCEL_EINVAL;

	vaccel_debug("Destroying resource %lld", model->resource->id);
	/* This will destroy the underlying resource and call our
	 * destructor callback */
	int ret = resource_destroy(model->resource);
	if (ret)
		vaccel_warn("Could not destroy resource");

	free(model->resource);

	return VACCEL_OK;
}

vaccel_id_t vaccel_single_model_get_id(const struct vaccel_single_model *model)
{
	if (!model || !model->resource)
		return -1;

	return model->resource->id;
}
