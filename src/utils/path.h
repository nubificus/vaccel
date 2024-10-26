// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Check if path is a url */
bool path_is_url(const char *path);

/* Generate path string from path parts.
 * Important: Last argument should always be NULL */
int path_init_from_parts(char *path, size_t size, const char *first_part, ...);

/* Allocate string and generate path from path parts.
 * Important: Last argument should always be NULL */
int path_from_parts(char **path, const char *first_part, ...);

/* Extract file name from path */
int path_file_name(char **name, const char *path);

/* Add random suffix to file name.
 * IMPORTANT: The resulting path must be used with fs_*_unique() functions
 * to actually add the random characters */
int path_file_name_add_random_suffix(char *path, size_t *ext_len,
				     const char *base_path, const char *suffix);

#ifdef __cplusplus
}
#endif
