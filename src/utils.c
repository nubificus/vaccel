// SPDX-License-Identifier: Apache-2.0

#include "utils.h"
#include "error.h"
#include "log.h"

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

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
	int fd;
	int ret;

	fd = open(path, O_RDONLY);
	if (fd == -1) {
		vaccel_error("Could not open file %s: %s", path,
			     strerror(errno));
		return errno;
	}

	/* Find the size of the file */
	struct stat stat;
	ret = fstat(fd, &stat);
	if (ret < 0) {
		vaccel_error("Could not fstat file %s: %s", path,
			     strerror(errno));
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
		ssize_t rret = read(fd, &buff[ptr], bytes);
		if (rret < 0) {
			vaccel_error("Could not read file %s: %s", path,
				     strerror(errno));
			free(buff);
			ret = errno;
			goto close_file;
		}

		ptr += rret;
		bytes -= rret;
	}

	*data = buff;
	*size = ptr;

close_file:
	close(fd);
	return ret;
}

int read_file_mmap(const char *path, void **data, size_t *size)
{
	int fd;
	int ret;

	fd = open(path, O_RDONLY);
	if (fd == -1) {
		vaccel_error("Could not open file %s: %s", path,
			     strerror(errno));
		return errno;
	}

	/* Find the size of the file */
	struct stat stat;
	ret = fstat(fd, &stat);
	if (ret < 0) {
		vaccel_error("Could not fstat file %s: %s", path,
			     strerror(errno));
		ret = errno;
		goto close_file;
	}
	if (!stat.st_size) {
		vaccel_error("File %s is empty", path);
		ret = VACCEL_EINVAL;
		goto close_file;
	}

	void *ptr = mmap(NULL, stat.st_size, PROT_READ | PROT_WRITE,
			 MAP_PRIVATE, fd, 0);
	if (!ptr) {
		vaccel_error("Could not mmap file %s: %s", path,
			     strerror(errno));
		ret = VACCEL_ENOMEM;
		goto close_file;
	}

	*data = ptr;
	*size = stat.st_size;

close_file:
	close(fd);
	return ret;
}
