// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "include/utils/path.h" // IWYU pragma: export
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Parse path string from URI */
int path_init_from_uri(char *path, size_t size, vaccel_path_t *type,
		       const char *uri);

/* Allocate string and parse path from URI */
int path_from_uri(char **path, vaccel_path_t *type, const char *uri);

/* Check if path is a url */
bool path_is_url(const char *path);

/* Determine type of path */
int path_type(const char *path, vaccel_path_t *type);

/* Generate path string from path parts.
 * IMPORTANT: Last argument should always be NULL */
int path_init_from_parts(char *path, size_t size, const char *first_part, ...);

/* Allocate string and generate path from path parts.
 * IMPORTANT: Last argument should always be NULL */
int path_from_parts(char **path, const char *first_part, ...);

/* Extract file name from path
 * If an alloc_name is provided the resulting name string will be allocated and
 * returned there. If not, the string will be written to name.
 * IMPORTANT: If alloc_name == NULL a name/size big enough to hold the file name
 * must be provided.*/
int path_file_name(const char *path, char *name, size_t size,
		   char **alloc_name);

/* Add random suffix to file name.
 * IMPORTANT: The resulting path must be used with fs_*_unique() functions
 * to actually add the random characters */
int path_file_name_add_random_suffix(char *path, size_t size, size_t *ext_len,
				     const char *base_path, const char *suffix);

#ifdef __cplusplus
}
#endif
