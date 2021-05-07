#include "resources.h"
#include "file.h"
#include "error.h"
#include "log.h"

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

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

	return VACCEL_OK;
}

int vaccel_file_persist(struct vaccel_file *file, struct vaccel_resource *res,
		const char *filename)
{
	int ret;

	vaccel_debug("Persisting file to resource");

	if (!file || !file->data | !file->size) {
		vaccel_error("Invalid file");
		return VACCEL_EINVAL;
	}

	if (!res) {
		vaccel_error("Invalid resource");
		return VACCEL_EINVAL;
	}

	if (!filename) {
		vaccel_error("You need to provide a name for the file");
		return VACCEL_EINVAL;
	}

	/* +1 for '\0' */
	int path_len = snprintf(NULL, 0, "%s/%s", res->rundir, filename) + 1;

	if (file->path) {
		vaccel_error("Found path for vAccel file. Not overwriting");
		return VACCEL_EEXISTS;
	}

	file->path = malloc(path_len);
	if (!file->path)
		return VACCEL_ENOMEM;

	/* No need to check that here, we know the length of the string */
	snprintf(file->path, path_len, "%s/%s", res->rundir, filename);
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
	return VACCEL_OK;

close_file:
	fclose(fp);
free_path:
	free(file->path);
	return ret;

}

int vaccel_file_from_buffer(struct vaccel_file *file, const uint8_t *buff,
		size_t size, const char *filename, struct vaccel_resource *res,
		bool persist)
{
	if (!file || !buff || !size)
		return VACCEL_EINVAL;

	file->path = NULL;
	file->data = (uint8_t *)buff;
	file->size = size;

	if (persist)
		return vaccel_file_persist(file, res, filename);

	return VACCEL_OK;
}

int vaccel_file_destroy(struct vaccel_file *file)
{
	if (!file)
		return VACCEL_EINVAL;

	vaccel_debug("Removing file %s", file->path);

	if (file->path_owned) {
		if (remove(file->path))
			vaccel_warn("Could not remove file from rundir: %s",
					file->path);
	}

	free(file->path);
	return VACCEL_OK;
}
