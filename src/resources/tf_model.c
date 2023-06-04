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

#include "resources/tf_model.h"
#include "resources.h"
#include "log.h"
#include "error.h"
#include "session.h"

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

static int tf_model_destructor(void *data)
{
	struct vaccel_tf_model *model = (struct vaccel_tf_model *)data;

	if (!model)
		return VACCEL_EINVAL;

	vaccel_file_destroy(&model->file);

	return VACCEL_OK;
}

int vaccel_tf_model_new(struct vaccel_tf_model *model, const char *path)
{
	struct vaccel_resource *res = malloc(sizeof(*res));
	if (!res)
		return VACCEL_ENOMEM;

	int ret = vaccel_file_new(&model->file, path);
	if (ret)
		goto free_resource;

	ret = resource_new(res, VACCEL_RES_TF_MODEL, (void *)model,
			tf_model_destructor);
	if (ret)
		goto destroy_file;

	model->resource = res;

	return VACCEL_OK;

destroy_file:
	vaccel_file_destroy(&model->file);
free_resource:
	free(res);
	return ret;
}

int vaccel_tf_model_new_from_buffer(struct vaccel_tf_model *model,
		const uint8_t *buff, size_t size)
{
	if (!model || !buff || !size)
		return VACCEL_EINVAL;

	struct vaccel_resource *res = malloc(sizeof(*res));
	if (!res)
		return VACCEL_ENOMEM;

	int ret = vaccel_file_from_buffer(&model->file, buff, size, NULL,
			NULL, false, false);
	if (ret)
		goto free_resource;

	ret = resource_new(res, VACCEL_RES_TF_MODEL, (void *)model,
			tf_model_destructor);
	if (ret)
		goto destroy_file;

	ret = resource_create_rundir(res);
	if (ret)
		goto destroy_resource;

	vaccel_debug("New rundir for resource %s", res->rundir);

	ret = vaccel_file_persist(&model->file, res->rundir, "model.pb", false);
	if (ret)
		goto destroy_resource;

	model->resource = res;

	return VACCEL_OK;

destroy_resource:
	resource_destroy(res);
destroy_file:
	vaccel_file_destroy(&model->file);
free_resource:
	free(res);

	return ret;
}
int vaccel_tf_model_destroy(struct vaccel_tf_model *model)
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

vaccel_id_t vaccel_tf_model_get_id(const struct vaccel_tf_model *model)
{
	if (!model || !model->resource)
		return -1;

	return model->resource->id;
}
