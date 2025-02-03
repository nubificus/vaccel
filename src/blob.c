// SPDX-License-Identifier: Apache-2.0

#define _POSIX_C_SOURCE 200809L

#include "blob.h"
#include "error.h"
#include "log.h"
#include "utils/fs.h"
#include "utils/path.h"
#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
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
int vaccel_blob_persist(struct vaccel_blob *blob, const char *dir,
			const char *filename, bool randomize)
{
	if (!blob || !blob->data || !blob->size ||
	    blob->type == VACCEL_BLOB_NONE) {
		vaccel_error("Invalid blob");
		return VACCEL_EINVAL;
	}

	if (!filename) {
		vaccel_error("You need to provide a name for the blob");
		return VACCEL_EINVAL;
	}

	if (!fs_path_is_dir(dir)) {
		vaccel_error("Invalid directory");
		return VACCEL_ENOENT;
	}

	if (blob->path) {
		vaccel_error("Found path for vAccel blob. Not overwriting");
		return VACCEL_EEXIST;
	}

	blob->path_owned = true;

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
		ret = fs_file_create_unique(fpath, 0, &blob->path, &fd);
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

			ret = fs_file_create_unique(fpath, 0, &blob->path, &fd);
			free(fpath);
			if (ret) {
				vaccel_error(
					"Could not create unique file for %s: %s",
					filename, strerror(ret));
				return ret;
			}
		} else {
			blob->path = fpath;
		}
	}

	/* Extract filename from path */
	ret = path_file_name(blob->path, NULL, 0, &blob->name);
	if (ret) {
		vaccel_error("Could not extract filename from %s", blob->path);
		close(fd);
		goto remove_file;
	}

	vaccel_debug("Persisting file %s to %s", blob->name, blob->path);

	/* Write file->data buffer to the new file */
	FILE *fp = fdopen(fd, "w+");
	if (!fp) {
		vaccel_error("Could not open file %s: %s", blob->path,
			     strerror(errno));
		close(fd);
		ret = VACCEL_EIO;
		goto free_name;
	}

	if (fwrite(blob->data, sizeof(char), blob->size, fp) != blob->size) {
		vaccel_error("Could not persist file %s: %s", blob->path,
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
	void *old_ptr = blob->data;
	size_t old_size = blob->size;
	ret = fs_file_read_mmap(blob->path, (void **)&blob->data, &blob->size);
	if (ret) {
		vaccel_error("Could not re-map file");
		blob->data = old_ptr;
		blob->size = old_size;
		goto free_name;
	}
	blob->type = VACCEL_BLOB_MAPPED;
	return VACCEL_OK;

free_name:
	free(blob->name);
	blob->name = NULL;
remove_file:
	if (fs_file_remove(blob->path))
		vaccel_warn("Could not remove file %s", blob->path);

	free(blob->path);
	blob->path = NULL;

	return ret;
}

/* Initialize a file resource from an existing file in the filesystem
 *
 * The path of the system will be copied.
 */
int vaccel_blob_init(struct vaccel_blob *blob, const char *path)
{
	if (!blob || !path)
		return VACCEL_EINVAL;

	if (!fs_path_is_file(path)) {
		vaccel_error("Invalid file %s", path);
		return VACCEL_EINVAL;
	}

	blob->path = strdup(path);
	if (!blob->path)
		return VACCEL_ENOMEM;

	int ret = path_file_name(blob->path, NULL, 0, &blob->name);
	if (ret) {
		vaccel_error("Could not extract filename from %s", blob->path);
		free(blob->path);
		return ret;
	}

	blob->type = VACCEL_BLOB_FILE;
	blob->path_owned = false;
	blob->data = NULL;
	blob->size = 0;

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
int vaccel_blob_init_from_buf(struct vaccel_blob *blob, const uint8_t *buf,
			      size_t size, const char *name, const char *dir,
			      bool randomize)
{
	if (!blob || !buf || !size)
		return VACCEL_EINVAL;

	if (!name) {
		vaccel_error("You need to provide a name for the blob");
		return VACCEL_EINVAL;
	}

	blob->name = NULL;
	blob->path = NULL;
	blob->path_owned = false;
	blob->data = (uint8_t *)buf;
	blob->size = size;
	blob->type = VACCEL_BLOB_BUF;

	if (dir)
		return vaccel_blob_persist(blob, dir, name, randomize);

	blob->name = strdup(name);
	if (!blob->name)
		return VACCEL_ENOMEM;

	return VACCEL_OK;
}

/* Release blob resources
 *
 * Releases any resources reserved for the file, If the file
 * has been persisted it will remove the file from the filesystem.
 * If the data of the file have been read in memory the memory
 * will be deallocated.
 */
int vaccel_blob_release(struct vaccel_blob *blob)
{
	if (!blob)
		return VACCEL_EINVAL;

	if (!vaccel_blob_initialized(blob)) {
		vaccel_error("Cannot release uninitialized file");
		return VACCEL_EINVAL;
	}

	if (blob->name)
		free(blob->name);
	blob->name = NULL;

	/* Just a file with data we got from the user. Nothing to do */
	if (!blob->path)
		return VACCEL_OK;

	/* There is a path in the disk representing the file,
	 * which means that if we hold a pointer to the contents
	 * of the file, this has been mapped, so unmap it */
	if (blob->type == VACCEL_BLOB_MAPPED && blob->data && blob->size) {
		int ret = munmap(blob->data, blob->size);
		if (ret) {
			vaccel_error("Failed to unmap file %s (size=%d): %s",
				     blob->path, blob->size, strerror(errno));
			return ret;
		}
	}
	blob->data = NULL;
	blob->size = 0;

	/* If we own the path to the file, just remove it from the
	 * file system */
	if (blob->type == VACCEL_BLOB_MAPPED && blob->path_owned) {
		vaccel_debug("Removing file %s", blob->path);
		if (fs_file_remove(blob->path))
			vaccel_warn("Could not remove file %s", blob->path);
	}
	blob->path_owned = false;

	free(blob->path);
	blob->path = NULL;
	blob->type = VACCEL_BLOB_NONE;

	return VACCEL_OK;
}

/* Allocate and initialize a blob resource from an existing file in the
 * filesystem
 *
 * This function will allocate a new blob struct before calling
 * vaccel_file_init(),
 */
int vaccel_blob_new(struct vaccel_blob **blob, const char *path)
{
	if (!blob)
		return VACCEL_EINVAL;

	struct vaccel_blob *b =
		(struct vaccel_blob *)malloc(sizeof(struct vaccel_blob));
	if (!b)
		return VACCEL_ENOMEM;

	int ret = vaccel_blob_init(b, path);
	if (ret) {
		free(b);
		return ret;
	}

	*blob = b;

	return VACCEL_OK;
}

/* Allocate and initialize a blob resource from in-memory data.
 *
 * This function will allocate a new blob struct before calling
 * vaccel_file_init_from_buf(),
 */
int vaccel_blob_from_buf(struct vaccel_blob **blob, const uint8_t *buf,
			 size_t size, const char *name, const char *dir,
			 bool randomize)
{
	if (!blob)
		return VACCEL_EINVAL;

	struct vaccel_blob *b =
		(struct vaccel_blob *)malloc(sizeof(struct vaccel_blob));
	if (!b)
		return VACCEL_ENOMEM;

	int ret = vaccel_blob_init_from_buf(b, buf, size, name, dir, randomize);
	if (ret) {
		free(b);
		return ret;
	}

	*blob = b;

	return VACCEL_OK;
}

/* Release blob resources and free file object
 *
 * This function will cleanup and free file objects created with
 * vaccel_file_new() or vaccel_file_from_*() functions.
 */
int vaccel_blob_delete(struct vaccel_blob *blob)
{
	int ret = vaccel_blob_release(blob);
	if (ret)
		return ret;

	free(blob);

	return VACCEL_OK;
}

/* Check if the blob has been initialized
 *
 * A file is initialized either if a path to the file
 * has been set or data has been loaded from memory
 */
bool vaccel_blob_initialized(struct vaccel_blob *blob)
{
	if (!blob || blob->type == VACCEL_BLOB_NONE)
		return false;

	return ((blob->type == VACCEL_BLOB_FILE && blob->path) ||
		(blob->type == VACCEL_BLOB_BUF && blob->data && blob->size) ||
		(blob->type == VACCEL_BLOB_MAPPED && blob->data && blob->size &&
		 blob->path));
}

/* Read the blob in-memory
 *
 * This reads the content of the file in memory, if it has not
 * been read already.
 */
int vaccel_blob_read(struct vaccel_blob *blob)
{
	if (!blob || blob->type == VACCEL_BLOB_NONE)
		return VACCEL_EINVAL;

	if (blob->data)
		return VACCEL_OK;

	if (!blob->path)
		return VACCEL_EINVAL;

	int ret = fs_file_read_mmap(blob->path, (void **)&blob->data,
				    &blob->size);
	if (ret == VACCEL_OK)
		blob->type = VACCEL_BLOB_MAPPED;
	return ret;
}

/* Get a pointer to the data of the file
 *
 * If the data have not been loaded to memory, this will
 * do so, through a call to `vaccel_file_read`.
 */
uint8_t *vaccel_blob_data(struct vaccel_blob *blob, size_t *size)
{
	if (!blob || blob->type == VACCEL_BLOB_NONE)
		return NULL;

	/* Make sure the data has been read in memory */
	if (!blob->data || !blob->size)
		if (vaccel_blob_read(blob))
			return NULL;

	if (size)
		*size = blob->size;

	return blob->data;
}

/* Get the path of the blob
 *
 * If the file is owned this will be the path of the file that
 * was created when persisted. Otherwise, it will be the path of
 * the file from which the vaccel_file was created
 */
const char *vaccel_blob_path(struct vaccel_blob *blob)
{
	if (!blob || !blob->path)
		return NULL;

	return (blob->type == VACCEL_BLOB_MAPPED ||
		blob->type == VACCEL_BLOB_FILE) ?
		       blob->path :
		       NULL;
}
