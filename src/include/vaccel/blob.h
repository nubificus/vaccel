// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "utils/enum.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Define vaccel_blob_type_t, vaccel_blob_type_to_str() and
 * vaccel_blob_type_to_base_str() */
#define _ENUM_PREFIX VACCEL_BLOB
#define VACCEL_BLOB_TYPE_ENUM_LIST(VACCEL_ENUM_ITEM) \
	VACCEL_ENUM_ITEM(FILE, 0, _ENUM_PREFIX)      \
	VACCEL_ENUM_ITEM(BUFFER, _ENUM_PREFIX)       \
	VACCEL_ENUM_ITEM(MAPPED, _ENUM_PREFIX)

VACCEL_ENUM_DEF_WITH_STR_FUNCS(vaccel_blob_type, _ENUM_PREFIX,
			       VACCEL_BLOB_TYPE_ENUM_LIST)
#undef _ENUM_PREFIX

struct vaccel_resource;

struct vaccel_blob {
	/* blob type */
	vaccel_blob_type_t type;

	/* name of the blob */
	char *name;

	/* path to the blob file; only set for non-buffer blobs */
	char *path;

	/* true if the struct owns the blob file */
	bool path_owned;

	/* data content of the blob; can be `NULL` if a blob file has not been
	 * read */
	uint8_t *data;

	/* true if the struct owns the data */
	bool data_owned;

	/* data size of the blob; can be `0` if a blob file has not been
	 * read */
	size_t size;
};

/* Persist a blob in the filesystem */
int vaccel_blob_persist(struct vaccel_blob *blob, const char *dir,
			const char *filename, bool randomize);

/* Initialize a blob from an existing file in the filesystem */
int vaccel_blob_init(struct vaccel_blob *blob, const char *path);

/* Initialize a blob from in-memory data */
int vaccel_blob_init_from_buf(struct vaccel_blob *blob, const uint8_t *buf,
			      size_t size, bool own, const char *filename,
			      const char *dir, bool randomize);

/* Release blob resources */
int vaccel_blob_release(struct vaccel_blob *blob);

/* Allocate and initialize a blob from an existing file in the filesystem */
int vaccel_blob_new(struct vaccel_blob **blob, const char *path);

/* Allocate and initialize a blob from in-memory data */
int vaccel_blob_from_buf(struct vaccel_blob **blob, const uint8_t *buf,
			 size_t size, bool own, const char *filename,
			 const char *dir, bool randomize);

/* Release blob resources and free object */
int vaccel_blob_delete(struct vaccel_blob *blob);

/* Read the blob in memory */
int vaccel_blob_read(struct vaccel_blob *blob);

/* Get the data of the blob */
uint8_t *vaccel_blob_data(struct vaccel_blob *blob, size_t *size);

/* Get the path of the blob */
const char *vaccel_blob_path(struct vaccel_blob *blob);

/* Check if a blob is valid */
bool vaccel_blob_valid(const struct vaccel_blob *blob);

#ifdef __cplusplus
}
#endif
