#include "resources.h"
#include "file.h"
#include "error.h"
#include "log.h"
#include "utils.h"

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <stdbool.h>

/* Create a file resource from an existing file in the filesystem
 *
 * The path of the system will be copied
 */
int vaccel_file_new(struct vaccel_file *file, const char *path)
{
	if (!file || !path)
		return VACCEL_EINVAL;

	if (access(path, R_OK)) {
		vaccel_warn("Cannot find file: %s", path);
		return errno;
	}

	file->path = strdup(path);
	if (!file->path)
		return VACCEL_ENOMEM;

	file->path_owned = false;
	file->data = NULL;
	file->size = 0;

	return VACCEL_OK;
}

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
		const char *filename)
{
	int ret;

	vaccel_debug("Persisting file");

	if (!file || !file->data | !file->size) {
		vaccel_error("Invalid file");
		return VACCEL_EINVAL;
	}

	if (!dir_exists(dir)) {
		vaccel_error("Invalid directory");
		return VACCEL_ENOENT;
	}

	if (!filename) {
		vaccel_error("You need to provide a name for the file");
		return VACCEL_EINVAL;
	}

	/* +1 for '\0' */
	int path_len = snprintf(NULL, 0, "%s/%s", dir, filename) + 1;

	if (file->path) {
		vaccel_error("Found path for vAccel file. Not overwriting");
		return VACCEL_EEXISTS;
	}

	file->path = malloc(path_len);
	if (!file->path)
		return VACCEL_ENOMEM;

	/* No need to check that here, we know the length of the string */
	snprintf(file->path, path_len, "%s/%s", dir, filename);
	file->path_owned = true;

	FILE *fp = fopen(file->path, "w+");
	if (!fp) {
		ret = errno;
		goto free_path;
	}

	size_t len = fwrite(file->data, sizeof(char), file->size, fp);
	if (len != file->size) {
		vaccel_error("Could not write to file %s", file->path);
		ret = VACCEL_EIO;
		goto close_file;
	}

	fclose(fp);

	/* We deallocate the initial pointer and mmap a new one,
	 * so that changes through the pointer are synced with the
	 * file */
	void *old_ptr = file->data;
	size_t old_size = file->size;
	ret = read_file(file->path, (void **)&file->data, &file->size);
	if (ret) {
		vaccel_debug("Could not re-map file");
		file->data = old_ptr;
		file->size = old_size;
		goto close_file;
	}
	
	return VACCEL_OK;

close_file:
	fclose(fp);
free_path:
	free(file->path);
	return ret;

}

/* Initialize a file from in-memory data.
 *
 * This will set the data of the file and it will persist
 * them in the filesystem if requested to do so.
 *
 * It takes ownership of the data, and it will deallocate
 * it when the file resource is destroyed.
 */
int vaccel_file_from_buffer(
	struct vaccel_file *file,
	const uint8_t *buff, size_t size,
	const char *filename,
	bool persist, const char *dir
) {
	if (!file || !buff || !size)
		return VACCEL_EINVAL;

	file->path = NULL;
	file->data = (uint8_t *)buff;
	file->size = size;

	if (persist)
		return vaccel_file_persist(file, dir, filename);

	return VACCEL_OK;
}

/* Destroy a file
 *
 * Releases any resources reserved for the file, If the file
 * has been persisted it will remove the file from the filesystem.
 * If the data of the file have been read in memory the memory
 * will be deallocated
 */
int vaccel_file_destroy(struct vaccel_file *file)
{
	if (!file)
		return VACCEL_EINVAL;

	if (file->path && file->path_owned) {
		vaccel_debug("Removing file %s", file->path);
		if (remove(file->path))
			vaccel_warn("Could not remove file from rundir: %s",
					file->path);
		free(file->path);
	}

	if (file->data)
		free(file->data);

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
	
	return read_file(file->path, (void **)&file->data, &file->size);
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
