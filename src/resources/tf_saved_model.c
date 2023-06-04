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

#include "resources/tf_saved_model.h"
#include "utils.h"
#include "error.h"
#include "log.h"
#include "resources.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <regex.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

#define MAX_PATH 1024

static int tf_model_destructor(void *data)
{
	struct vaccel_tf_saved_model *model =
		(struct vaccel_tf_saved_model *)data;
	if (!model)
		return VACCEL_EINVAL;

	vaccel_file_destroy(&model->model);
	vaccel_file_destroy(&model->checkpoint);
	vaccel_file_destroy(&model->var_index);

	struct vaccel_resource *res = model->resource;
	if (!res || !res->rundir)
		return VACCEL_OK;

	char path[MAX_PATH];
	if (snprintf(path, MAX_PATH, "%s/variables", res->rundir) >= MAX_PATH) {
		vaccel_warn("Path too long");
		return VACCEL_ENAMETOOLONG;
	}

	if (dir_exists(path))
		rmdir(path);

	return VACCEL_OK;
}

static int file_from_regex(struct vaccel_file *file, const char *dirpath,
		const char *pattern)
{
	regex_t r;
	char path[MAX_PATH];
	int ret = VACCEL_OK;

	if (regcomp(&r, pattern, REG_EXTENDED | REG_NOSUB))
		return VACCEL_EINVAL;

	DIR *dir = opendir(dirpath);
	if (!dir) {
		ret = VACCEL_EINVAL;
		goto cleanup_regex;
	}

	struct dirent *dent;
	while ((dent = readdir(dir))) {
		if (!regexec(&r, dent->d_name, 0, 0, 0)) {
			if (snprintf(path, MAX_PATH,
				"%s/%s", dirpath, dent->d_name) >= MAX_PATH) {
				ret = VACCEL_ENAMETOOLONG;
				break;
			}

			ret = vaccel_file_new(file, path);
			break;
		}
	}

	closedir(dir);
cleanup_regex:
	regfree(&r);

	return ret;
}

/* Allocate a new SavedModel descriptor */
struct vaccel_tf_saved_model *vaccel_tf_saved_model_new(void)
{
	struct vaccel_tf_saved_model *new = calloc(1, sizeof(*new));
	if (!new)
		vaccel_warn("Could not allocate memory");

	return new;
}


/* Set the export directory of a SavedModel
 *
 * Used to create a SavedModel from an export path. The call
 * will check for the existence of the directory and it will
 * look for the relevant files under that path, i.e. the protobuf
 * description of the graph and the checkpoint and variable index
 * files under the `variables` directory. However, it will not
 * check for the validity of these files.
 *
 * If the exported model is malformed this will not fail. We will
 * find that out when trying to actually load the model
 */
int vaccel_tf_saved_model_set_path(
	struct vaccel_tf_saved_model *model,
	const char *path
) {
	char var_path[MAX_PATH];

	if (!model)
		return VACCEL_EINVAL;

	if (!dir_exists(path))
		return VACCEL_EINVAL;

	if (snprintf(var_path, MAX_PATH, "%s/variables", path) >= MAX_PATH) {
		vaccel_warn("Path too long");
		return VACCEL_ENAMETOOLONG;
	}

	/* Find the protobuf file in the top-level directory */
	int ret = file_from_regex(&model->model, path, "saved_model.pb$");
	if (ret) {
		vaccel_warn("Could not find model file");
		return VACCEL_ENOENT;
	}

	/* Find the checkpoint under the `variables` directory */
	ret = file_from_regex(&model->checkpoint, var_path, "variables.data-*");
	if (ret) {
		vaccel_warn("Could not find checkpoint file");
		goto destroy_model_file;
	}

	/* Find the variables index file under the `variables` directory */
	ret = file_from_regex(&model->var_index, var_path, "variables.index");
	if (ret) {
		vaccel_warn("Could not find variables index file");
		goto destroy_ckpt_file;
	}

	model->path = strdup(path);
	if (!model->path) {
		ret = VACCEL_ENOMEM;
		goto destroy_var_index_file;
	}

	model->resource = NULL;
	vaccel_debug("Set TensorFlow model path to %s", model->path);

	return VACCEL_OK;

destroy_var_index_file:
	vaccel_file_destroy(&model->var_index);
destroy_ckpt_file:
	vaccel_file_destroy(&model->checkpoint);
destroy_model_file:
	vaccel_file_destroy(&model->model);

	return ret;
}

/* Get the path of the export directory
 *
 * This will be NULL if the path is not owned
 */
const char *vaccel_tf_saved_model_get_path(struct vaccel_tf_saved_model *model)
{
	if (!model)
		return NULL;

	return model->path;
}

/* Set the protobuf file from in-memory data
 *
 * Create the underlying file resource for the protobuf file
 * with the Graph description. This will take ownership of the
 * buffer passed as an argument.
 */
int vaccel_tf_saved_model_set_model(
	struct vaccel_tf_saved_model *model,
	const uint8_t *ptr, size_t len
) {
	vaccel_debug("Setting protobuf file for model");

	if (!model) {
		vaccel_warn("Invalid model");
		return VACCEL_EINVAL;
	}

	if (!ptr || !len) {
		vaccel_warn("Invalid data for protobuf model");
		return VACCEL_EINVAL;
	}

	int ret = vaccel_file_from_buffer(&model->model, ptr, len,
			NULL, false, NULL, false);
	return ret;
}

/* Get a pointer to the data of the protobuf Graph model
 *
 * This will return a pointer to the data of the protobuf Graph file.
 * If the data have not been read before, this will first try to
 * read the data (mmap them in memory) before returning.
 */
const uint8_t *vaccel_tf_saved_model_get_model(
	struct vaccel_tf_saved_model *model,
	size_t *len
) {
	if (!model)
		return NULL;

	// vaccel_file_data will read the file in memory if
	// not already done
	return vaccel_file_data(&model->model, len);
}

/* Set the checkpoint file from in-memory data
 *
 * Create the underlying file resource for the variables checkpoint
 * file. This will take ownership of the buffer passed as an argument.
 */
int vaccel_tf_saved_model_set_checkpoint(
	struct vaccel_tf_saved_model *model,
	const uint8_t *ptr, size_t len
) {
	vaccel_debug("Setting checkpoint file for model");

	if (!model) {
		vaccel_warn("Invalid model");
		return VACCEL_EINVAL;
	}

	if (!ptr || !len) {
		vaccel_warn("Invalid data for checkpoint");
		return VACCEL_EINVAL;
	}

	int ret = vaccel_file_from_buffer(&model->checkpoint, ptr, len,
			NULL, false, NULL, false);
	return ret;
}

/* Get a pointer to the data of the checkpoint
 * 
 * This will return a pointer to the data of the checkpoint file.
 * If the data have not been read before, this will first try to
 * read the data (mmap them in memory) before returning.
 */
const uint8_t *vaccel_tf_saved_model_get_checkpoint(
	struct vaccel_tf_saved_model *model,
	size_t *len
) {
	if (!model)
		return NULL;

	// vaccel_file_data will read the file in memory if
	// not already done
	return vaccel_file_data(&model->checkpoint, len);
}

/* Set the variable index file from in-memory data
 *
 * Create the underlying file resource for the variables index
 * file. This will take ownership of the buffer passed as an argument.
 */
int vaccel_tf_saved_model_set_var_index(
	struct vaccel_tf_saved_model *model,
	const uint8_t *ptr, size_t len
) {
	vaccel_debug("Setting variables index file for model");

	if (!model) {
		vaccel_warn("Invalid model");
		return VACCEL_EINVAL;
	}

	if (!ptr || !len) {
		vaccel_warn("Invalid data for variables index");
		return VACCEL_EINVAL;
	}

	int ret = vaccel_file_from_buffer(&model->var_index, ptr, len,
			NULL, false, NULL, false);
	return ret;
}

/* Get a pointer to the data of the checkpoint
 * 
 * This will return a pointer to the data of the checkpoint file.
 * If the data have not been read before, this will first try to
 * read the data (mmap them in memory) before returning.
 */
const uint8_t *vaccel_tf_saved_model_get_var_index(
	struct vaccel_tf_saved_model *model,
	size_t *len
) {
	if (!model)
		return NULL;

	// vaccel_file_data will read the file in memory if
	// not already done
	return vaccel_file_data(&model->var_index, len);
}

/* Register the file as a vAccel resource
 *
 * If the SavedModel is created from in-memory data, this will first
 * create the SavedModel export directory structure and persist the files
 * before creating the resource.
 */
int vaccel_tf_saved_model_register(struct vaccel_tf_saved_model *model)
{
	char var_dir[MAX_PATH];

	vaccel_debug("Registering new vAccel TensorFlow model");

	if (!(vaccel_file_initialized(&model->model)
		&& vaccel_file_initialized(&model->checkpoint)
		&& vaccel_file_initialized(&model->var_index))) {
		vaccel_warn("Will not register uninitialized resource");
		return VACCEL_EINVAL;
	}

	struct vaccel_resource *res = malloc(sizeof(*model->resource));
	if (!res) {
		vaccel_warn("Could not allocate memory");
		return VACCEL_ENOMEM;
	}

	int ret = resource_new(res, VACCEL_RES_TF_MODEL, (void *)model,
			tf_model_destructor);
	if (ret)
		goto free_resource;

	/* If we have created the model from a path in the disk we are
	 * done here */
	if (model->path)
		goto out;
	
	/* Else we need to persist the files in the disk */
	ret = resource_create_rundir(res);
	if (ret)
		goto destroy_resource;

	/* Create the variables directory */
	ret = snprintf(var_dir, MAX_PATH, "%s/variables", res->rundir);
	if (ret >= MAX_PATH) {
		ret = VACCEL_ENAMETOOLONG;
		goto destroy_resource;
	}

	ret = mkdir(var_dir, 0700);
	if (ret) {
		ret = errno;
		goto destroy_resource;
	}

	ret = vaccel_file_persist(&model->model, res->rundir,
			"saved_model.pb", false);
	if (ret)
		goto destroy_resource;

	ret = vaccel_file_persist(&model->checkpoint, var_dir,
			"variables.data-00000-of-00001", false);
	if (ret)
		goto destroy_resource;

	ret = vaccel_file_persist(&model->var_index, var_dir,
			"variables.index", false);
	if (ret)
		goto destroy_resource;

	model->path = res->rundir;
out:
	vaccel_debug("New resource %lu", res->id);
	model->resource = res;

	return VACCEL_OK;

destroy_resource:
	resource_destroy(res);
free_resource:
	free(res);

	return ret;
}

/* Destroy the SavedModel resource
 *
 * This will handle the destruction of the underlying resource and
 * release any resources allocated for the model, i.e. delete the model
 * directory if owned and deallocate the files.
 */
int vaccel_tf_saved_model_destroy(struct vaccel_tf_saved_model *model)
{
	if (!model)
		return VACCEL_EINVAL;

	vaccel_debug("Destroying resource %u", model->resource->id);
	/* This will destroy the underlying resource and call our
	 * destructor callback */
	int ret = resource_destroy(model->resource);
	if (ret)
		vaccel_warn("Could not destroy resource");

	free(model->resource);

	return VACCEL_OK;
}

/* Get the id of the SavedModel */
vaccel_id_t vaccel_tf_saved_model_id(const struct vaccel_tf_saved_model *model)
{
	if (!model || !model->resource)
		return -1;

	return model->resource->id;
}
