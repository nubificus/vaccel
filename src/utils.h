// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Check if a directory exists */
bool dir_exists(const char *path);

/* It tries to cleanup a rundir created by vaccel. It will
 * return VACCEL_OK at success, or a proper error code otherwise
 */
int cleanup_rundir(const char *path);

/* Read a file into a buffer */
int read_file(const char *path, void **data, size_t *size);

/* Read a file into a buffer with mmap.
 * This will actually mmap the file with read and write access
 * and return the mapped memory and the size of the file */
int read_file_mmap(const char *path, void **data, size_t *size);

#ifdef __cplusplus
}
#endif
