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
		vaccel_debug("Could not open file %s", path);
		return errno;
	}

	/* Find the size of the file */
	struct stat stat;
	ret = fstat(fd, &stat);
	if (ret < 0) {
		vaccel_debug("Could not debug file %s", strerror(errno));
		ret = errno;
		goto close_file;
	}

	void *ptr = mmap(NULL, stat.st_size, PROT_READ|PROT_WRITE, MAP_PRIVATE,
			fd, 0);
	if (!ptr) {
		vaccel_debug("Could not mmap file");
		ret = VACCEL_ENOMEM;
		goto close_file;
	}

	*data = ptr;
	*size = stat.st_size;

close_file:
	close(fd);
	return ret;
}
