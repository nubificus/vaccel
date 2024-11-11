// SPDX-License-Identifier: Apache-2.0

#define _POSIX_C_SOURCE 200809L

#include "error.h"
#include "file.h"
#include "log.h"
#include "utils/fs.h"
#include "utils/path.h"
#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

/* Persist a file in the filesystem
 *
 * For files that have been initialized from in-memory data, this
 * will persist them in the filesystem under the requested directory
 * using the provided filename.
 *
 * It will fail if the file has been initialized through an existing
 * path in the filesystem.
 */
int vaccel_file_persist(struct vaccel_file *file, const char *dir,
			const char *filename, bool randomize)
{
	if (!file || !file->data || !file->size) {
		vaccel_error("Invalid file");
		return VACCEL_EINVAL;
	}

	if (!filename) {
		vaccel_error("You need to provide a name for the file");
		return VACCEL_EINVAL;
	}

	if (!fs_path_is_dir(dir)) {
		vaccel_error("Invalid directory");
		return VACCEL_ENOENT;
	}

	if (file->path) {
		vaccel_error("Found path for vAccel file. Not overwriting");
		return VACCEL_EEXIST;
	}

	file->path_owned = true;

	/* Generate fs path string */
	char *fpath = NULL;
	int ret = path_from_parts(&fpath, dir, filename, NULL);
	if (ret) {
		vaccel_error("Could not generate %s path string", filename);
		return ret;
	}

	int fd;
	if (randomize) {
		/* Create unique (random) file in the fs */
		ret = fs_file_create_unique(fpath, 0, &file->path, &fd);
		free(fpath);
		if (ret) {
			vaccel_error("Could not create unique file for %s: %s",
				     filename, strerror(ret));
			return ret;
		}
	} else {
		/* Create file in the fs */
		ret = fs_file_create(fpath, &fd);
		if (ret && ret != VACCEL_EEXIST) {
			vaccel_error("Could not create file %s", fpath);
			free(fpath);
			return ret;
		}

		/* If file exists in the fs create a unique file */
		if (ret == VACCEL_EEXIST) {
			vaccel_warn("File %s/%s exists", dir, filename);
			vaccel_warn("Adding a random value to the filename");

			ret = fs_file_create_unique(fpath, 0, &file->path, &fd);
			free(fpath);
			if (ret) {
				vaccel_error(
					"Could not create unique file for %s: %s",
					filename, strerror(ret));
				return ret;
			}
		} else {
			file->path = fpath;
		}
	}

	/* Extract filename from path */
	ret = path_file_name(file->path, NULL, 0, &file->name);
	if (ret) {
		vaccel_error("Could not extract filename from %s", file->path);
		close(fd);
		goto remove_file;
	}

	vaccel_debug("Persisting file %s to %s", file->name, file->path);

	/* Write file->data buffer to the new file */
	FILE *fp = fdopen(fd, "w+");
	if (!fp) {
		vaccel_error("Could not open file %s: %s", file->path,
			     strerror(errno));
		close(fd);
		ret = VACCEL_EIO;
		goto free_name;
	}

	if (fwrite(file->data, sizeof(char), file->size, fp) != file->size) {
		vaccel_error("Could not persist file %s: %s", file->path,
			     strerror(errno));
		fclose(fp);
		ret = VACCEL_EIO;
		goto free_name;
	}

	/* fwrite() is a buffered operation so we need to fclose() here.
	 * If we don't, we will have to fflush() to ensure data has been written
	 * to disk before we try to mmap */
	fclose(fp);

	/* Deallocate the initial pointer and mmap a new one,
	 * so that changes through the pointer are synced with the
	 * file */
	void *old_ptr = file->data;
	size_t old_size = file->size;
	ret = fs_file_read_mmap(file->path, (void **)&file->data, &file->size);
	if (ret) {
		vaccel_error("Could not re-map file");
		file->data = old_ptr;
		file->size = old_size;
		goto free_name;
	}

	return VACCEL_OK;

free_name:
	free(file->name);
	file->name = NULL;
remove_file:
	if (fs_file_remove(file->path))
		vaccel_warn("Could not remove file %s", file->path);

	free(file->path);
	file->path = NULL;

	return ret;
}

/* Initialize a file resource from an existing file in the filesystem
 *
 * The path of the system will be copied.
 */
int vaccel_file_init(struct vaccel_file *file, const char *path)
{
	if (!file || !path)
		return VACCEL_EINVAL;

	if (!fs_path_is_file(path)) {
		vaccel_error("Invalid file %s", path);
		return VACCEL_EINVAL;
	}

	file->path = strdup(path);
	if (!file->path)
		return VACCEL_ENOMEM;

	int ret = path_file_name(file->path, NULL, 0, &file->name);
	if (ret) {
		vaccel_error("Could not extract filename from %s", file->path);
		free(file->path);
		return ret;
	}

	file->path_owned = false;
	file->data = NULL;
	file->size = 0;

	return VACCEL_OK;
}

/* Initialize a file from in-memory data.
 *
 * This will set the data of the file and it will persist
 * them in the filesystem if the relevant dir is provided.
 *
 * It does not take ownership of the data pointer, but the user is responsible
 * of making sure that the memory it points to outlives the `vaccel_file`
 * resource.
 */
int vaccel_file_init_from_buf(struct vaccel_file *file, const uint8_t *buf,
			      size_t size, const char *filename,
			      const char *dir, bool randomize)
{
	if (!file || !buf || !size)
		return VACCEL_EINVAL;

	if (!filename) {
		vaccel_error("You need to provide a name for the file");
		return VACCEL_EINVAL;
	}

	file->name = NULL;
	file->path = NULL;
	file->path_owned = false;
	file->data = (uint8_t *)buf;
	file->size = size;

	if (dir)
		return vaccel_file_persist(file, dir, filename, randomize);

	file->name = strdup(filename);
	if (!file->name)
		return VACCEL_ENOMEM;

	return VACCEL_OK;
}

/* Release file resources
 *
 * Releases any resources reserved for the file, If the file
 * has been persisted it will remove the file from the filesystem.
 * If the data of the file have been read in memory the memory
 * will be deallocated.
 */
int vaccel_file_release(struct vaccel_file *file)
{
	if (!file)
		return VACCEL_EINVAL;

	if (!vaccel_file_initialized(file)) {
		vaccel_error("Cannot release uninitialized file");
		return VACCEL_EINVAL;
	}

	if (file->name)
		free(file->name);
	file->name = NULL;

	/* Just a file with data we got from the user. Nothing to do */
	if (!file->path)
		return VACCEL_OK;

	/* There is a path in the disk representing the file,
	 * which means that if we hold a pointer to the contents
	 * of the file, this has been mapped, so unmap it */
	if (file->data && file->size) {
		int ret = munmap(file->data, file->size);
		if (ret) {
			vaccel_error("Failed to unmap file %s (size=%d): %s",
				     file->path, file->size, strerror(errno));
			return ret;
		}
	}
	file->data = NULL;
	file->size = 0;

	/* If we own the path to the file, just remove it from the
	 * file system */
	if (file->path_owned) {
		vaccel_debug("Removing file %s", file->path);
		if (fs_file_remove(file->path))
			vaccel_warn("Could not remove file %s", file->path);
	}
	file->path_owned = false;

	free(file->path);
	file->path = NULL;

	return VACCEL_OK;
}

/* Allocate and initialize a file resource from an existing file in the
 * filesystem
 *
 * This function will allocate a new file struct before calling
 * vaccel_file_init(),
 */
int vaccel_file_new(struct vaccel_file **file, const char *path)
{
	if (!file)
		return VACCEL_EINVAL;

	struct vaccel_file *f =
		(struct vaccel_file *)malloc(sizeof(struct vaccel_file));
	if (!f)
		return VACCEL_ENOMEM;

	int ret = vaccel_file_init(f, path);
	if (ret) {
		free(f);
		return ret;
	}

	*file = f;

	return VACCEL_OK;
}

/* Allocate and initialize a file resource from in-memory data.
 *
 * This function will allocate a new file struct before calling
 * vaccel_file_init_from_buf(),
 */
int vaccel_file_from_buf(struct vaccel_file **file, const uint8_t *buf,
			 size_t size, const char *filename, const char *dir,
			 bool randomize)
{
	if (!file)
		return VACCEL_EINVAL;

	struct vaccel_file *f =
		(struct vaccel_file *)malloc(sizeof(struct vaccel_file));
	if (!f)
		return VACCEL_ENOMEM;

	int ret = vaccel_file_init_from_buf(f, buf, size, filename, dir,
					    randomize);
	if (ret) {
		free(f);
		return ret;
	}

	*file = f;

	return VACCEL_OK;
}

/* Release file resources and free file object
 *
 * This function will cleanup and free file objects created with
 * vaccel_file_new() or vaccel_file_from_*() functions.
 */
int vaccel_file_delete(struct vaccel_file *file)
{
	int ret = vaccel_file_release(file);
	if (ret)
		return ret;

	free(file);

	return VACCEL_OK;
}

/* Check if the file has been initialized
 *
 * A file is initialized either if a path to the file
 * has been set or data has been loaded from memory
 */
bool vaccel_file_initialized(struct vaccel_file *file)
{
	return file && (file->path || (file->data && file->size));
}

/* Read the file in-memory
 *
 * This reads the content of the file in memory, if it has not
 * been read already.
 */
int vaccel_file_read(struct vaccel_file *file)
{
	if (!file)
		return VACCEL_EINVAL;

	if (file->data)
		return VACCEL_OK;

	if (!file->path)
		return VACCEL_EINVAL;

	return fs_file_read_mmap(file->path, (void **)&file->data, &file->size);
}

/* Get a pointer to the data of the file
 *
 * If the data have not been loaded to memory, this will
 * do so, through a call to `vaccel_file_read`.
 */
uint8_t *vaccel_file_data(struct vaccel_file *file, size_t *size)
{
	if (!file)
		return NULL;

	/* Make sure the data has been read in memory */
	if (!file->data || !file->size)
		if (vaccel_file_read(file))
			return NULL;

	if (size)
		*size = file->size;

	return file->data;
}

/* Get the path of the file
 *
 * If the file is owned this will be the path of the file that
 * was created when persisted. Otherwise, it will be the path of
 * the file from which the vaccel_file was created
 */
const char *vaccel_file_path(struct vaccel_file *file)
{
	if (!file)
		return NULL;

	return file->path;
}
