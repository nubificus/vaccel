// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "vaccel_id.h"
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

struct vaccel_resource;

struct vaccel_file {
	/* name of the file */
	char *name;

	/* Path to file */
	char *path;

	/* Do we own the file? */
	bool path_owned;

	/* Pointer to the contents of the file in case we hold them
	 * in a buffer */
	uint8_t *data;
	size_t size;
};

int vaccel_file_new(struct vaccel_file *file, const char *path);
int vaccel_file_from_buffer(struct vaccel_file *file, const uint8_t *buff,
			    size_t size, const char *filename, bool persist,
			    const char *dir, bool randomize);
int vaccel_file_persist(struct vaccel_file *file, const char *dir,
			const char *filename, bool randomize);
int vaccel_file_destroy(struct vaccel_file *file);
bool vaccel_file_initialized(struct vaccel_file *file);
int vaccel_file_read(struct vaccel_file *file);
uint8_t *vaccel_file_data(struct vaccel_file *file, size_t *size);
const char *vaccel_file_path(struct vaccel_file *file);

#ifdef __cplusplus
}
#endif
