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
