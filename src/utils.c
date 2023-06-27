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

#include "utils.h"
#include "error.h"
#include "log.h"

#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <stdbool.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <sys/mman.h>

bool dir_exists(const char *path)
{
	DIR *dir = opendir(path);
	if (!dir)
		return false;

	closedir(dir);
	return true;
}

int cleanup_rundir(const char *path)
{
	/* At the moment, just assume that the rundir is empty, i.e. all
	 * contents have been cleaned up by the code that owns it */
	if (rmdir(path))
		return errno;

	return VACCEL_OK;
}

int read_file(const char *path, void **data, size_t *size)
{
	int fd, ret;

	fd = open(path, O_RDONLY);
	if (fd == -1) {
		vaccel_error("Could not open file %s: %s", path, strerror(errno));
		return errno;
	}

	/* Find the size of the file */
	struct stat stat;
	ret = fstat(fd, &stat);
	if (ret < 0) {
		vaccel_error("Could not fstat file %s: %s", path, strerror(errno));
		ret = errno;
		goto close_file;
	}
	if (!stat.st_size) {
		vaccel_error("File %s is empty", path);
		ret = VACCEL_EINVAL;
		goto close_file;
	}

	unsigned char *buff = malloc(stat.st_size);
	if (!buff) {
		vaccel_error("Could not malloc buff: %s", strerror(errno));
		ret = errno;
		goto close_file;
	}

	size_t bytes = stat.st_size;
	ssize_t ptr = 0;
	while (bytes) {
		ssize_t ret = read(fd, &buff[ptr], bytes);
		if (ret < 0) {
			vaccel_error("Could not read file %s: %s", path, strerror(errno));
			free(buff);
			ret = errno;
			goto close_file;
		}

		ptr += ret;
		bytes -= ret;
	}

	*data = buff;
	*size = ptr;

close_file:
	close(fd);
	return ret;
}

int read_file_mmap(const char *path, void **data, size_t *size)
{
	int fd, ret;

	fd = open(path, O_RDONLY);
	if (fd == -1) {
		vaccel_error("Could not open file %s: %s", path, strerror(errno));
		return errno;
	}

	/* Find the size of the file */
	struct stat stat;
	ret = fstat(fd, &stat);
	if (ret < 0) {
		vaccel_error("Could not fstat file %s: %s", path, strerror(errno));
		ret = errno;
		goto close_file;
	}
	if (!stat.st_size) {
		vaccel_error("File %s is empty", path);
		ret = VACCEL_EINVAL;
		goto close_file;
	}

	void *ptr = mmap(NULL, stat.st_size, PROT_READ|PROT_WRITE, MAP_PRIVATE,
			fd, 0);
	if (!ptr) {
		vaccel_error("Could not mmap file %s: %s", path, strerror(errno));
		ret = VACCEL_ENOMEM;
		goto close_file;
	}

	*data = ptr;
	*size = stat.st_size;

close_file:
	close(fd);
	return ret;
}
