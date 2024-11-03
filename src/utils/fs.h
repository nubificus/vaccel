// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int (*fs_path_callback_t)(const char *path, int idx, va_list args);

/* Check if path exists */
bool fs_path_exists(const char *path);

/* Check if path is an existing directory */
bool fs_path_is_dir(const char *path);

/* Check if path is an existing file */
bool fs_path_is_file(const char *path);

/* Process files in directory path recursively with func */
int fs_dir_process_files(const char *path, fs_path_callback_t func, ...);

/* Create a directory in the fs recursively (if it does't exist) */
int fs_dir_create(const char *path);

/* Create a unique directory (with random suffix) in the fs.
 * If a final_path is provided the resulting path string will be allocated and
 * returned there. If not, the suffix will be appended to base_path.
 * IMPORTANT:
 * - If final_path == NULL a path/size big enough to hold the suffixed path must
 *   be provided.
 * - If the path ends with a trailing slash a sub-directory with a random name
 *   will be created under the path
 * NOTE: base_path will be created recursively if it does not exist in the fs */
int fs_dir_create_unique(char *path, size_t size, char **final_path);

/* Remove a directory if empty */
int fs_dir_remove(const char *path);

/* Create a file in the fs
 * IMPORTANT: The parent directories of path must exist in the fs */
int fs_file_create(const char *path, int *fd);

/* Create a unique file (with random suffix) in the fs.
 * If a final_path is provided the resulting path string will be allocated and
 * returned there. If not, the suffix will be appended to path.
 * If fd is provided it will be used to return a fd of an open file.
 * IMPORTANT:
 * - If final_path == NULL a path/size big enough to hold the suffixed path must
 *   be provided.
 * - The parent directories of the path must exist in the fs
 * NOTE: The random suffix will be added before the file extension */
int fs_file_create_unique(char *path, size_t size, char **final_path, int *fd);

/* Remove a path */
int fs_file_remove(const char *path);

/* Read a file into a buffer */
int fs_file_read(const char *path, void **data, size_t *size);

/* Read a file into a buffer with mmap.
 * This will actually mmap the file with read and write access
 * and return the mapped memory and the size of the file */
int fs_file_read_mmap(const char *path, void **data, size_t *size);

#ifdef __cplusplus
}
#endif
