// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <stdbool.h>
#include <stddef.h>

#include <vaccel_file.h>
#include <vaccel_id.h>

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

int vaccel_single_model_set_path(struct vaccel_single_model *model,
				 const char *path);

const char *vaccel_single_model_get_path(struct vaccel_single_model *model);

int vaccel_single_model_set_file(struct vaccel_single_model *model,
				 const char *filename, const uint8_t *ptr,
				 size_t len);

const uint8_t *vaccel_single_model_get_file(struct vaccel_single_model *model,
					    size_t *len);

int vaccel_single_model_register(struct vaccel_single_model *model);

int vaccel_single_model_destroy(struct vaccel_single_model *model);

vaccel_id_t vaccel_single_model_get_id(const struct vaccel_single_model *model);

vaccel_id_t
vaccel_single_model_get_remote_id(const struct vaccel_single_model *model);

#ifdef __cplusplus
}
#endif
