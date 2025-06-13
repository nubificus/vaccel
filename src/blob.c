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

/* Persist a blob in the filesystem.
 *
 * For blobs that have been initialized from in-memory data, this
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
	    blob->type >= VACCEL_BLOB_MAX) {
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

	if (!blob->name || (blob->name && strcmp(blob->name, filename) != 0)) {
		if (blob->name) {
			free(blob->name);
			blob->name = NULL;
		}
		ret = path_file_name(blob->path, NULL, 0, &blob->name);
		if (ret) {
			vaccel_error("Could not extract filename from %s",
				     blob->path);
			close(fd);
			goto remove_file;
		}
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

	/* If the blob data was owned, free the allocated memory */
	if (blob->type == VACCEL_BLOB_BUFFER && blob->data_owned)
		free(old_ptr);

	/* The file is mmap'ed to data so the relevant memory is always owned */
	blob->data_owned = true;
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

/* Initialize a blob from an existing file in the filesystem.
 *
 * The file path will be copied and stored.
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
	blob->data_owned = false;
	blob->data = NULL;
	blob->size = 0;

	return VACCEL_OK;
}

/* Initialize a blob from in-memory data.
 *
 * This will set the data of the blob and persist them in a file in the
 * filesystem if the relevant dir is provided.
 *
 * If `own` is 'true' the data will not be owned and the user is responsible
 * for ensuring the provided memory outlives the `vaccel_blob` instance.
 */
int vaccel_blob_init_from_buf(struct vaccel_blob *blob, const uint8_t *buf,
			      size_t size, bool own, const char *name,
			      const char *dir, bool randomize)
{
	int ret;
	if (!blob || !buf || !size)
		return VACCEL_EINVAL;

	if (!name) {
		vaccel_error("You need to provide a name for the blob");
		return VACCEL_EINVAL;
	}

	blob->data = (uint8_t *)buf;
	blob->data_owned = false;
	blob->name = NULL;
	blob->path = NULL;
	blob->path_owned = false;
	blob->size = size;
	blob->type = VACCEL_BLOB_BUFFER;

	if (!dir) {
		if (own) {
			blob->data = (uint8_t *)malloc(size);
			if (!blob->data) {
				ret = VACCEL_ENOMEM;
				goto err;
			}
			memcpy(blob->data, buf, size);
			blob->data_owned = own;
		}

		blob->name = strdup(name);
		if (!blob->name) {
			ret = VACCEL_ENOMEM;
			goto err;
		}
	} else {
		ret = vaccel_blob_persist(blob, dir, name, randomize);
		if (ret)
			goto err;
	}

	return VACCEL_OK;
err:
	if (blob->data_owned && blob->data)
		free(blob->data);
	blob->data = NULL;
	blob->data_owned = false;
	blob->size = 0;
	blob->type = VACCEL_BLOB_MAX;
	return ret;
}

/* Release blob resources.
 *
 * Releases any resources reserved for the blob.
 *
 * If the blob has been persisted this will remove the file from the filesystem.
 * If the data of the file have been read in memory, the memory will be
 * deallocated.
 */
int vaccel_blob_release(struct vaccel_blob *blob)
{
	if (!blob)
		return VACCEL_EINVAL;

	if (blob->type >= VACCEL_BLOB_MAX) {
		vaccel_error("Cannot release uninitialized blob");
		return VACCEL_EINVAL;
	}

	if (blob->name)
		free(blob->name);
	blob->name = NULL;

	if (blob->data && blob->size && blob->data_owned) {
		if (blob->type == VACCEL_BLOB_MAPPED) {
			int ret = munmap(blob->data, blob->size);
			if (ret) {
				vaccel_error(
					"Failed to unmap file %s (size=%d): %s",
					blob->path, blob->size,
					strerror(errno));
				return ret;
			}
		}

		if (blob->type == VACCEL_BLOB_BUFFER)
			free(blob->data);
	}
	blob->data = NULL;
	blob->size = 0;
	blob->data_owned = false;

	if (blob->path) {
		/* If we own the path to the file, remove it from the
		 * filesystem */
		if (blob->path_owned) {
			vaccel_debug("Removing file %s", blob->path);
			if (fs_file_remove(blob->path))
				vaccel_warn("Could not remove file %s",
					    blob->path);
		}
		free(blob->path);
	}
	blob->path = NULL;
	blob->path_owned = false;

	blob->type = VACCEL_BLOB_MAX;

	return VACCEL_OK;
}

/* Allocate and initialize a blob from an existing file in the filesystem.
 *
 * This function will allocate a new blob struct before calling
 * `vaccel_blob_init()`.
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

/* Allocate and initialize a blob from in-memory data.
 *
 * This function will allocate a new blob struct before calling
 * `vaccel_blob_init_from_buf()`.
 */
int vaccel_blob_from_buf(struct vaccel_blob **blob, const uint8_t *buf,
			 size_t size, bool own, const char *name,
			 const char *dir, bool randomize)
{
	if (!blob)
		return VACCEL_EINVAL;

	struct vaccel_blob *b =
		(struct vaccel_blob *)malloc(sizeof(struct vaccel_blob));
	if (!b)
		return VACCEL_ENOMEM;

	int ret = vaccel_blob_init_from_buf(b, buf, size, own, name, dir,
					    randomize);
	if (ret) {
		free(b);
		return ret;
	}

	*blob = b;

	return VACCEL_OK;
}

/* Release blob resources and free object.
 *
 * This function will cleanup and free objects created with `vaccel_blob_new()`
 * or `vaccel_blob_from_*()` functions.
 */
int vaccel_blob_delete(struct vaccel_blob *blob)
{
	int ret = vaccel_blob_release(blob);
	if (ret)
		return ret;

	free(blob);

	return VACCEL_OK;
}

/* Read the blob in memory.
 *
 * This reads the content of the file in memory if it has not already been read.
 */
int vaccel_blob_read(struct vaccel_blob *blob)
{
	if (!blob || blob->type >= VACCEL_BLOB_MAX)
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

/* Get the data of the blob.
 *
 * If the data have not been loaded to memory, this will do so through a call to
 * `vaccel_file_read()`.
 */
uint8_t *vaccel_blob_data(struct vaccel_blob *blob, size_t *size)
{
	if (!blob || blob->type >= VACCEL_BLOB_MAX)
		return NULL;

	/* Make sure the data has been read in memory */
	if (!blob->data || !blob->size)
		if (vaccel_blob_read(blob))
			return NULL;

	if (size)
		*size = blob->size;

	return blob->data;
}

/* Get the path of the blob.
 *
 * If the file is owned this will be the path of the file that was created when
 * persisted. Otherwise, it will be the path of the file from which the
 * `vaccel_blob` was created.
 * If the blob is not backed by a file, `NULL` will be returned.
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
