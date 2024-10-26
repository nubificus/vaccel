// SPDX-License-Identifier: Apache-2.0

#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE

#include "error.h"
#include "fs.h"
#include "log.h"
#include "path.h"
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <limits.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

bool fs_path_exists(const char *path)
{
	if (!path)
		return false;

	struct stat st;
	return stat(path, &st) == 0;
}

bool fs_path_is_dir(const char *path)
{
	if (!path)
		return false;

	struct stat st;
	return (stat(path, &st) == 0) && S_ISDIR(st.st_mode);
}

bool fs_path_is_file(const char *path)
{
	if (!path)
		return false;

	struct stat st;
	return (stat(path, &st) == 0) && S_ISREG(st.st_mode);
}

int fs_dir_num_files(const char *path)
{
	if (!path) {
		vaccel_error("Invalid path");
		return VACCEL_EINVAL;
	}

	DIR *d = opendir(path);
	if (!d) {
		vaccel_error("Could not open dir %s: %s", path,
			     strerror(errno));
		return VACCEL_EINVAL;
	}

	struct dirent *dir;
	int nr_files = 0;
	while ((dir = readdir(d)) != NULL) {
		if (dir->d_type == DT_REG)
			++nr_files;
	}

	closedir(d);

	return nr_files;
}

int fs_dir_create(const char *path)
{
	if (!path)
		return VACCEL_EINVAL;

	if (strlen(path) + 1 > PATH_MAX) {
		vaccel_error("Path %s name too long", path);
		return VACCEL_ENAMETOOLONG;
	}

	if (fs_path_is_dir(path))
		return VACCEL_EEXIST;

	if (fs_path_exists(path)) {
		vaccel_error("Path %s exists but is not a directory", path);
		return VACCEL_ENOTDIR;
	}

	char temp_path[PATH_MAX];
	strncpy(temp_path, path, PATH_MAX - 1);
	temp_path[sizeof(temp_path) - 1] = '\0';

	/* Iterate path and create directories if they don't exist */
	char *pos = temp_path;
	do {
		/* Find next slash */
		char *next_slash = strchr(pos, '/');

		/* Temporarily terminate string to create intermediate
		 * directories */
		if (next_slash)
			*next_slash = '\0';

		/* Create directory if it does not exist */
		if (strlen(temp_path) > 0 && !fs_path_exists(temp_path)) {
			int ret = mkdir(temp_path, S_IRWXU);
			if (ret && errno != EEXIST) {
				vaccel_error("Could not create dir %s: %s",
					     temp_path, strerror(errno));
				return errno;
			}
		}

		/* Restore slash for the next iteration or break */
		if (next_slash) {
			*next_slash = '/';
			pos = next_slash + 1;
		} else {
			break;
		}
	} while (1);

	return VACCEL_OK;
}

int fs_dir_create_unique(char *path, size_t size, char **final_path)
{
	if (!path || (!final_path && !size))
		return VACCEL_EINVAL;

	size_t path_len = strlen(path);
	if (path_len == 0) {
		vaccel_error("Path cannot be empty");
		return VACCEL_EINVAL;
	}

	const char *suffix;
	if (path[path_len - 1] == '/')
		suffix = "XXXXXX";
	else
		suffix = "_XXXXXX";
	/* +2 for '/' and '\0' */
	size_t final_path_len = path_len + strlen(suffix) + 2;

	if (final_path_len > PATH_MAX) {
		vaccel_error("Unique path name too long for %s", path);
		return VACCEL_ENAMETOOLONG;
	}

	/* Duplicate path so we can modify it and be able to write to it while
	 * reading from it using snprintf */
	char dup_path[PATH_MAX];
	strncpy(dup_path, path, PATH_MAX - 1);
	dup_path[PATH_MAX - 1] = '\0';

	char *in_path;
	char *parent_path = NULL;
	/* Set parent path */
	if (dup_path[path_len - 1] == '/') {
		/* Remove trailing slash if present */
		dup_path[path_len - 1] = '\0';
		in_path = dup_path;
		parent_path = in_path;
	} else if (final_path != NULL) {
		/* If we allocate a new final path we can use path directly */
		in_path = path;
		parent_path = dirname(dup_path);
	} else {
		/* If we return the final path directly we need to do one more
		 * path copy for dirname() */
		char temp_path[PATH_MAX];
		strncpy(temp_path, dup_path, PATH_MAX - 1);
		temp_path[PATH_MAX - 1] = '\0';

		in_path = dup_path;
		parent_path = dirname(temp_path);
	}

	/* Ensure parent path exists */
	int ret = fs_dir_create(parent_path);
	if (ret && ret != VACCEL_EEXIST)
		return ret;

	/* If final_path is provided allocate memory, else
	 * modify path with the random suffix */
	char *out_path = path;
	if (final_path != NULL) {
		*final_path = malloc(final_path_len);
		if (!*final_path)
			return VACCEL_ENOMEM;
		out_path = *final_path;
	} else if (size < final_path_len) {
		vaccel_error("path buffer does not fit random suffix for %s",
			     path);
		return VACCEL_EINVAL;
	}

	/* Generate unique path string. If a trailing slash existed in the
	 * original path re-add it */
	if (path[path_len - 1] == '/')
		ret = snprintf(out_path, final_path_len, "%s/%s", in_path,
			       suffix);
	else
		ret = snprintf(out_path, final_path_len, "%s%s", in_path,
			       suffix);
	if (ret < 0) {
		vaccel_error("Could not generate unique path for %s: %s", path,
			     strerror(errno));
		goto free;
	}
	if ((size_t)ret >= final_path_len) {
		vaccel_error("Unique path for %s too long", path);
		ret = VACCEL_ENAMETOOLONG;
		goto free;
	}

	/* Create the unique directory */
	if (!mkdtemp(out_path)) {
		vaccel_error("Could not create unique path for %s: %s", path,
			     strerror(errno));
		ret = errno;
		goto free;
	}

	return VACCEL_OK;

free:
	if (final_path && *final_path) {
		free(*final_path);
		*final_path = NULL;
	}

	return ret;
}

int fs_dir_remove(const char *path)
{
	if (rmdir(path))
		return errno;

	return VACCEL_OK;
}

int fs_file_create(const char *path, int *fd)
{
	if (!path)
		return VACCEL_EINVAL;

	if (strlen(path) + 1 > PATH_MAX) {
		vaccel_error("Path %s name too long", path);
		return VACCEL_ENAMETOOLONG;
	}

	if (fs_path_is_file(path))
		return VACCEL_EEXIST;

	if (fs_path_exists(path)) {
		vaccel_error("Path %s exists but is not a file", path);
		return VACCEL_EINVAL;
	}

	/* Open file with O_CREAT | O_EXCL to ensure it doesn't overwrite
	 * an existing one */
	int ffd = open(path, O_CREAT | O_RDWR | O_EXCL, S_IRWXU);
	if (ffd < 0) {
		vaccel_error("Could not open file %s: %s", path,
			     strerror(errno));
		return errno;
	}

	if (fd != NULL)
		*fd = ffd;
	else {
		if (close(ffd) < 0) {
			vaccel_error("Could not close file %s: %s", path,
				     strerror(errno));
			return errno;
		}
	}

	return VACCEL_OK;
}

int fs_file_create_unique(char *path, size_t size, char **final_path, int *fd)
{
	if (!path || (!final_path && !size))
		return VACCEL_EINVAL;

	const char *suffix = "_XXXXXX";
	/* +1 for '\0' */
	size_t path_len = strlen(path) + strlen(suffix) + 1;

	if (path_len > PATH_MAX) {
		vaccel_error("Unique path name too long for %s", path);
		return VACCEL_ENAMETOOLONG;
	}

	/* If final_path is provided allocate memory, else
	 * modify path with the random suffix path */
	char *out_path = path;
	char *in_path = path;
	if (final_path != NULL) {
		*final_path = malloc(path_len);
		if (!*final_path)
			return VACCEL_ENOMEM;
		out_path = *final_path;
	} else {
		if (size < path_len) {
			vaccel_error(
				"Random suffix does not fit in path buffer for %s",
				path);
			return VACCEL_EINVAL;
		}
		in_path = strndup(path, size);
	}

	/* Insert random suffix to filename */
	size_t ext_len;
	int ret = path_file_name_add_random_suffix(out_path, &ext_len, in_path,
						   suffix);
	if (ret)
		goto free;

	/* Create the unique file */
	int ffd;
	if (ext_len > 0)
		ffd = mkstemps(out_path, ext_len);
	else
		ffd = mkstemp(out_path);
	if (ffd < 0) {
		vaccel_error("Could not create file %s: %s", out_path,
			     strerror(errno));
		ret = errno;
		goto free;
	}

	if (fd != NULL)
		*fd = ffd;
	else {
		if (close(ffd) < 0) {
			vaccel_error("Could not close file %s: %s", path,
				     strerror(errno));
			ret = errno;
			goto free;
		}
	}

	if (final_path == NULL)
		free(in_path);

	return VACCEL_OK;

free:
	if (final_path != NULL && *final_path != NULL) {
		free(*final_path);
		*final_path = NULL;
	} else {
		free(in_path);
	}

	return ret;
}

int fs_file_remove(const char *path)
{
	if (remove(path))
		return errno;

	return VACCEL_OK;
}

int fs_file_read(const char *path, void **data, size_t *size)
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

int fs_file_read_mmap(const char *path, void **data, size_t *size)
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
