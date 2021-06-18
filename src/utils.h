#ifndef __UTILS_H__
#define __UTILS_H__

#include <stdbool.h>
#include <stddef.h>

/* Check if a directory exists */
bool dir_exists(const char *path);

/* It tries to cleanup a rundir created by vaccelrt. It will
 * return VACCEL_OK at success, or a proper error code otherwise
 */
int cleanup_rundir(const char *path);

/* Read a file into a buffer
 *
 * This will actually mmap the file with read and write
 * access and it will reteurn the mapped memory and the
 * size of the file */
int read_file(const char *path, void **data, size_t *size);

#endif /* __UTILS_H__ */
