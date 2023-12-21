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

#pragma once

#include <stddef.h>
#include <stdbool.h>

#include <vaccel_id.h>
#include <vaccel_file.h>

#ifdef __cplusplus
extern "C" {
#endif

struct vaccel_resource;

/* A struct representing a single-file model
 */
struct vaccel_single_model {
	/* Underlying resource object */
	struct vaccel_resource *resource;

	/* The path of the model in disk */
	const char *path;

	/* The filename used to persist the file */
	const char *filename;

	/* The file representation of the model */
	struct vaccel_file file;

	/* Plugin specific data */
	void *plugin_data;
};

struct vaccel_single_model *vaccel_single_model_new(void);

int vaccel_single_model_set_path(
	struct vaccel_single_model *model,
	const char *path
);

const char *vaccel_single_model_get_path(struct vaccel_single_model *model);

int vaccel_single_model_set_file(
	struct vaccel_single_model *model, const char *filename,
	const uint8_t *ptr, size_t len
);

const uint8_t *vaccel_single_model_get_file(
	struct vaccel_single_model *model,
	size_t *len
);

int vaccel_single_model_register(struct vaccel_single_model *model);

int vaccel_single_model_destroy(struct vaccel_single_model *model);

vaccel_id_t vaccel_single_model_get_id(const struct vaccel_single_model *model);

#ifdef __cplusplus
}
#endif
