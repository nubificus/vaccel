// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "utils/enum.h"
#include "utils/path.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Define vaccel_blob_type_t, vaccel_blob_type_to_str() and
 * vaccel_blob_type_to_base_str() */
#define _ENUM_PREFIX VACCEL_BLOB
#define VACCEL_BLOB_TYPE_ENUM_LIST(VACCEL_ENUM_ITEM) \
	VACCEL_ENUM_ITEM(NONE, 0, _ENUM_PREFIX)      \
	VACCEL_ENUM_ITEM(FILE, _ENUM_PREFIX)         \
	VACCEL_ENUM_ITEM(BUF, _ENUM_PREFIX)          \
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

	/* Path to blob file */
	char *path;

	/* Do we own the file? */
	bool path_owned;

	/* Pointer to the contents of the blob in case we hold them
	 * in a buffer */
	uint8_t *data;
	size_t size;
};

int vaccel_blob_persist(struct vaccel_blob *blob, const char *dir,
			const char *filename, bool randomize);
int vaccel_blob_init(struct vaccel_blob *blob, const char *path);
int vaccel_blob_init_from_buf(struct vaccel_blob *blob, const uint8_t *buf,
			      size_t size, const char *filename,
			      const char *dir, bool randomize);
int vaccel_blob_release(struct vaccel_blob *blob);
int vaccel_blob_new(struct vaccel_blob **blob, const char *path);
int vaccel_blob_from_buf(struct vaccel_blob **blob, const uint8_t *buf,
			 size_t size, const char *filename, const char *dir,
			 bool randomize);
int vaccel_blob_delete(struct vaccel_blob *blob);
bool vaccel_blob_initialized(struct vaccel_blob *blob);
int vaccel_blob_read(struct vaccel_blob *blob);
uint8_t *vaccel_blob_data(struct vaccel_blob *blob, size_t *size);
const char *vaccel_blob_path(struct vaccel_blob *blob);

#ifdef __cplusplus
}
#endif
