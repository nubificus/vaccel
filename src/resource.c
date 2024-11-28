// SPDX-License-Identifier: Apache-2.0

#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE

#include "resource.h"
#include "error.h"
#include "file.h"
#include "id_pool.h"
#include "list.h"
#include "log.h"
#include "plugin.h"
#include "utils/fs.h"
#include "utils/net.h"
#include "utils/path.h"
#include <assert.h>
#include <dirent.h>
#include <inttypes.h>
#include <libgen.h>
#include <limits.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

enum { VACCEL_RESOURCES_MAX = 2048 };

static bool initialized = false;
static id_pool_t id_pool;

/* All the live (created) vAccel resources.
 * At the moment, this is an array where each element is a list of all
 * resources of the same time. We should think the data structure again.
 */
static list_t live_resources[VACCEL_RESOURCE_MAX];

int resources_bootstrap(void)
{
	int ret = id_pool_init(&id_pool, VACCEL_RESOURCES_MAX);
	if (ret)
		return ret;

	for (int i = 0; i < VACCEL_RESOURCE_MAX; ++i)
		list_init(&live_resources[i]);

	initialized = true;

	return VACCEL_OK;
}

int resources_cleanup(void)
{
	if (!initialized)
		return VACCEL_OK;

	for (int i = 0; i < VACCEL_RESOURCE_MAX; ++i) {
		struct vaccel_resource *res;
		struct vaccel_resource *tmp;
		for_each_vaccel_resource_safe(res, tmp, &live_resources[i])
			vaccel_resource_release(res);
	}

	initialized = false;

	return id_pool_release(&id_pool);
}

int vaccel_resource_get_by_id(struct vaccel_resource **resource, vaccel_id_t id)
{
	if (!initialized)
		return VACCEL_EPERM;

	for (int i = 0; i < VACCEL_RESOURCE_MAX; ++i) {
		struct vaccel_resource *res;
		struct vaccel_resource *tmp;
		for_each_vaccel_resource_safe(res, tmp, &live_resources[i]) {
			if (id == res->id) {
				*resource = res;
				return VACCEL_OK;
			}
		}
	}

	return VACCEL_EINVAL;
}

static void get_resource_id(struct vaccel_resource *res)
{
	res->id = id_pool_get(&id_pool);
}

static void put_resource_id(struct vaccel_resource *res)
{
	if (id_pool_put(&id_pool, res->id))
		vaccel_warn("Could not return resource ID to pool");
	res->id = -1;
}

void resource_refcount_inc(struct vaccel_resource *res)
{
	if (!res) {
		vaccel_error("BUG! Refcounting invalid resource");
		return;
	}

	atomic_fetch_add(&res->refcount, 1);
}

void resource_refcount_dec(struct vaccel_resource *res)
{
	if (!res) {
		vaccel_error("BUG! Refcounting invalid resource");
		return;
	}

	atomic_fetch_sub(&res->refcount, 1);
}

long int vaccel_resource_refcount(const struct vaccel_resource *res)
{
	if (!res) {
		vaccel_error("BUG! Refcounting invalid resource");
		return -VACCEL_EINVAL;
	}

	return atomic_load(&res->refcount);
}

int resource_create_rundir(struct vaccel_resource *res)
{
	if (!res || !res->id) {
		vaccel_error(
			"BUG! Trying to create rundir for invalid resource");
		return VACCEL_EINVAL;
	}

	char res_dir[NAME_MAX];
	int ret = snprintf(res_dir, NAME_MAX, "resource.%" PRId64, res->id);
	if (ret < 0) {
		vaccel_error("Could not generate resource %" PRId64
			     " rundir name",
			     res->id);
		return ret;
	}
	if (ret == NAME_MAX) {
		vaccel_error("Resource " PRId64 " rundir name too long",
			     res->id);
		return VACCEL_ENAMETOOLONG;
	}

	char *rundir;
	ret = path_from_parts(&rundir, vaccel_rundir(), res_dir, NULL);
	if (ret) {
		vaccel_error(
			"Could not generate rundir path for resource %" PRId64,
			res->id);
		return ret;
	}

	ret = fs_dir_create(rundir);
	if (ret) {
		vaccel_error("Could not create rundir for resource %" PRId64,
			     res->id);
		free(rundir);
		return ret;
	}

	res->rundir = rundir;

	vaccel_debug("New rundir for resource %" PRId64 ": %s", res->id,
		     res->rundir);

	return VACCEL_OK;
}

void resource_destroy_rundir(struct vaccel_resource *res)
{
	if (fs_dir_remove(res->rundir))
		vaccel_warn("Could not remove rundir %s: %s", res->rundir,
			    strerror(errno));

	free(res->rundir);
	res->rundir = NULL;
}

static void delete_files(struct vaccel_file **files, size_t nr_files)
{
	if (!files)
		return;

	for (size_t i = 0; i < nr_files; i++) {
		if (!files[i])
			continue;

		int ret = vaccel_file_delete(files[i]);
		if (ret) {
			vaccel_warn("Could not delete file %zu", i);
			continue;
		}
		files[i] = NULL;
	}
}

static int resource_populate_file_data(struct vaccel_resource *res)
{
	if (!res || !res->files || !res->nr_files)
		return VACCEL_EINVAL;

	for (size_t i = 0; i < res->nr_files; i++) {
		if (!res->files[i])
			return VACCEL_EINVAL;

		int ret = vaccel_file_read(res->files[i]);
		if (ret)
			return ret;
	}

	return VACCEL_OK;
}

static int add_dir_files_callback(const char *path, int idx, va_list args)
{
	if (!path || idx < 0)
		return VACCEL_EINVAL;

	struct vaccel_resource *res = va_arg(args, struct vaccel_resource *);
	if (res == NULL)
		return VACCEL_EINVAL;
	int nr_files = va_arg(args, int);
	if (nr_files <= 0)
		return VACCEL_EINVAL;

	if (idx >= nr_files) {
		vaccel_error(
			"Files to process are more than expected (%s vs %zu)",
			idx, nr_files);
		return VACCEL_EINVAL;
	}

	int ret = vaccel_file_new(&res->files[idx], path);
	if (ret)
		vaccel_error("Could not create vaccel_file for %s", path);

	return ret;
}

static int resource_add_files_from_dir(struct vaccel_resource *res,
				       bool with_data)
{
	if (!res)
		return VACCEL_EINVAL;

	/* If files exist only populate the file data if chosen */
	if (res->files && res->nr_files) {
		if (with_data)
			return resource_populate_file_data(res);
		return VACCEL_OK;
	}

	if (!res->paths || !res->paths[0] || res->nr_paths != 1)
		return VACCEL_EINVAL;

	/* Get number of dir files */
	int nr_dirfiles = fs_dir_process_files(res->paths[0], NULL);
	if (nr_dirfiles < 0) {
		vaccel_error("Could not get number of files in %s",
			     res->paths[0]);
		return VACCEL_EINVAL;
	}

	res->files = (struct vaccel_file **)malloc(
		nr_dirfiles * sizeof(struct vaccel_file *));
	if (!res->files)
		return VACCEL_ENOMEM;

	for (int i = 0; i < nr_dirfiles; i++)
		res->files[i] = NULL;

	/* Create vaccel_file struct for file paths. If the files are in
	 * subdirectories they will be persisted in a flat directory remotely */
	int ret = fs_dir_process_files(res->paths[0], add_dir_files_callback,
				       res, nr_dirfiles, NULL);
	if (ret < 0) {
		vaccel_error("Could not process files from dir %s",
			     res->paths[0]);
		goto free;
	}

	assert(nr_dirfiles == ret);
	res->nr_files = nr_dirfiles;

	if (with_data) {
		ret = resource_populate_file_data(res);
		if (ret) {
			vaccel_error("Could not populate resource file data");
			goto free;
		}
	}

	return VACCEL_OK;

free:
	delete_files(res->files, res->nr_files);
	free(res->files);
	res->files = NULL;
	res->nr_files = 0;

	return ret;
}

static int download_file(const char *path, const char *fs_dir,
			 size_t fs_path_size, char *fs_path)
{
	char filename[NAME_MAX];
	int ret = path_file_name(path, filename, NAME_MAX, NULL);
	if (ret)
		return ret;

	ret = path_init_from_parts(fs_path, fs_path_size, fs_dir, filename,
				   NULL);
	if (ret)
		return ret;

	ret = net_file_download(path, fs_path);
	if (ret) {
		if (ret == VACCEL_EREMOTEIO)
			fs_file_remove(fs_path);
		return ret;
	}

	return ret;
}

static int resource_add_files(struct vaccel_resource *res, bool remote,
			      bool download)
{
	if (!res)
		return VACCEL_EINVAL;

	if (res->files && res->nr_files)
		return VACCEL_OK;

	if (!res->paths || !res->nr_paths)
		return VACCEL_EINVAL;

	if (remote && !download)
		return VACCEL_OK;

	int ret;
	if (!res->rundir) {
		ret = resource_create_rundir(res);
		if (ret)
			goto cleanup_rundir;
	}

	res->files = (struct vaccel_file **)malloc(
		res->nr_paths * sizeof(struct vaccel_file *));
	if (!res->files)
		return VACCEL_ENOMEM;

	for (size_t i = 0; i < res->nr_paths; i++)
		res->files[i] = NULL;

	size_t nr_files = 0;
	for (size_t i = 0; i < res->nr_paths; i++) {
		if (!res->paths[i]) {
			ret = VACCEL_EINVAL;
			goto free;
		}

		if (remote) {
			char path[PATH_MAX];
			ret = download_file(res->paths[i], res->rundir,
					    PATH_MAX, path);
			if (ret) {
				vaccel_error("Could not download file from %s",
					     res->paths[i]);
				goto free;
			}

			ret = vaccel_file_new(&res->files[i], path);
			if (ret) {
				vaccel_error(
					"Could not create vaccel_file for %s",
					path);
				goto free;
			}
			/* Mark the path as owned so it will be deleted on release */
			res->files[i]->path_owned = true;
		} else {
			ret = vaccel_file_new(&res->files[i], res->paths[i]);
			if (ret) {
				vaccel_error(
					"Could not create vaccel_file for %s",
					res->paths[i]);
				goto free;
			}
		}

		++nr_files;
	}
	assert(res->nr_paths == nr_files);
	res->nr_files = nr_files;

	return VACCEL_OK;

free:
	delete_files(res->files, nr_files);
	free(res->files);
	res->files = NULL;
	res->nr_files = 0;
cleanup_rundir:
	resource_destroy_rundir(res);

	return ret;
}

static int resource_add_files_from_local(struct vaccel_resource *res,
					 bool with_data)
{
	if (!res)
		return VACCEL_EINVAL;

	int ret = resource_add_files(res, false, false);
	if (ret)
		return ret;

	if (with_data)
		return resource_populate_file_data(res);

	return VACCEL_OK;
}

static int resource_add_files_from_remote(struct vaccel_resource *res,
					  bool download)
{
	if (!res)
		return VACCEL_EINVAL;

	return resource_add_files(res, true, download);
}

static int resource_init_with_paths(struct vaccel_resource *res,
				    vaccel_resource_t type)
{
	if (!res || !res->paths || !res->nr_paths || !res->paths[0] ||
	    type >= VACCEL_RESOURCE_MAX)
		return VACCEL_EINVAL;

	res->remote_id = -1;
	res->type = type;
	res->files = NULL;
	res->nr_files = 0;
	res->rundir = NULL;
	atomic_init(&res->refcount, 0);

	list_init_entry(&res->entry);
	list_add_tail(&live_resources[res->type], &res->entry);

	vaccel_debug("Initialized resource %" PRId64, res->id);

	return VACCEL_OK;
}

static int resource_init_with_files(struct vaccel_resource *res,
				    vaccel_resource_t type)
{
	if (!res || !res->files || !res->nr_files ||
	    type >= VACCEL_RESOURCE_MAX || !res->rundir || !res->id)
		return VACCEL_EINVAL;

	res->remote_id = -1;
	res->type = type;
	res->path_type = VACCEL_PATH_LOCAL_FILE;
	res->paths = NULL;
	res->nr_paths = 0;
	atomic_init(&res->refcount, 0);

	list_init_entry(&res->entry);
	list_add_tail(&live_resources[res->type], &res->entry);

	vaccel_debug("Initialized resource %" PRId64, res->id);

	return VACCEL_OK;
}

int vaccel_resource_init(struct vaccel_resource *res, const char *path,
			 vaccel_resource_t type)
{
	return vaccel_resource_init_multi(res, &path, 1, type);
}

int vaccel_resource_init_multi(struct vaccel_resource *res, const char **paths,
			       size_t nr_paths, vaccel_resource_t type)
{
	int ret;

	if (!initialized)
		return VACCEL_EPERM;

	if (!res || !paths || !nr_paths || type >= VACCEL_RESOURCE_MAX)
		return VACCEL_EINVAL;

	get_resource_id(res);
	if (res->id < 0)
		return -(int)res->id;

	res->paths = (char **)malloc(nr_paths * sizeof(char *));
	if (!res->paths) {
		ret = VACCEL_ENOMEM;
		goto release_id;
	}

	for (size_t i = 0; i < nr_paths; i++)
		res->paths[i] = NULL;

	if (!paths[0]) {
		ret = VACCEL_EINVAL;
		goto free;
	}
	ret = path_from_uri(&res->paths[0], &res->path_type, paths[0]);
	if (ret) {
		vaccel_error("Could not parse URI for %s", paths[0]);
		goto free;
	}

	for (size_t i = 1; i < nr_paths; i++) {
		if (!paths[i]) {
			ret = VACCEL_EINVAL;
			goto free;
		}

		vaccel_path_t type;
		ret = path_from_uri(&res->paths[i], &type, paths[i]);
		if (ret) {
			vaccel_error("Could not parse URI for %s", paths[i]);
			goto free;
		}
		if (type == VACCEL_PATH_LOCAL_DIR) {
			vaccel_error(
				"Resources with multiple directory paths are not supported");
			ret = VACCEL_ENOTSUP;
			goto free;
		}
		if (type != res->path_type) {
			vaccel_error(
				"Resource paths cannot be of different types");
			ret = VACCEL_EINVAL;
			goto free;
		}
	}
	res->nr_paths = nr_paths;

	ret = resource_init_with_paths(res, type);
	if (ret) {
		vaccel_error("Could not initialize resource");
		goto free;
	}

	return VACCEL_OK;

free:
	for (size_t i = 0; i < nr_paths; i++) {
		if (!res->paths[i])
			continue;

		free(res->paths[i]);
	}
	free(res->paths);
	res->paths = NULL;
	res->nr_paths = 0;
release_id:
	put_resource_id(res);

	return ret;
}

int vaccel_resource_init_from_buf(struct vaccel_resource *res, const void *buf,
				  size_t nr_bytes, vaccel_resource_t type,
				  const char *filename)
{
	int ret;

	if (!initialized)
		return VACCEL_EPERM;

	if (!res || !buf || !nr_bytes || type >= VACCEL_RESOURCE_MAX)
		return VACCEL_EINVAL;

	get_resource_id(res);
	if (res->id < 0)
		return -(int)res->id;

	res->files =
		(struct vaccel_file **)malloc(sizeof(struct vaccel_file *));
	if (!res->files) {
		ret = VACCEL_ENOMEM;
		goto release_id;
	}

	ret = resource_create_rundir(res);
	if (ret)
		goto free_files;

	if (filename != NULL) {
		ret = vaccel_file_from_buf(&res->files[0], buf, nr_bytes,
					   filename, res->rundir, false);
	} else {
		ret = vaccel_file_from_buf(&res->files[0], buf, nr_bytes,
					   "file", res->rundir, true);
	}
	if (ret) {
		vaccel_error("Could not create vaccel_file from buffer");
		goto cleanup_rundir;
	}
	res->nr_files = 1;

	ret = resource_init_with_files(res, type);
	if (ret) {
		vaccel_error("Could not initialize resource");
		goto delete_file;
	}

	return VACCEL_OK;

delete_file:
	delete_files(res->files, res->nr_files);
cleanup_rundir:
	resource_destroy_rundir(res);
free_files:
	free(res->files);
	res->files = NULL;
	res->nr_files = 0;
release_id:
	put_resource_id(res);

	return ret;
}

int vaccel_resource_init_from_files(struct vaccel_resource *res,
				    const struct vaccel_file **files,
				    size_t nr_files, vaccel_resource_t type)
{
	int ret;

	if (!initialized)
		return VACCEL_EPERM;

	if (!res || !files || !nr_files || type >= VACCEL_RESOURCE_MAX)
		return VACCEL_EINVAL;

	get_resource_id(res);
	if (res->id < 0)
		return -(int)res->id;

	res->files = (struct vaccel_file **)malloc(
		nr_files * sizeof(struct vaccel_file *));
	if (!res->files) {
		ret = VACCEL_ENOMEM;
		goto release_id;
	}

	for (size_t i = 0; i < nr_files; i++)
		res->files[i] = NULL;

	ret = resource_create_rundir(res);
	if (ret)
		goto free_files;

	size_t nr_r_files = 0;
	for (size_t i = 0; i < nr_files; i++) {
		if (!files[i] || !files[i]->size || !files[i]->data ||
		    !files[i]->name) {
			ret = VACCEL_EINVAL;
			goto free;
		}

		ret = vaccel_file_from_buf(&res->files[i], files[i]->data,
					   files[i]->size, files[i]->name,
					   res->rundir, false);
		if (ret) {
			vaccel_error(
				"Could not create vaccel_file from buffer");
			goto free;
		}

		++nr_r_files;
	}
	assert(nr_r_files == nr_files);
	res->nr_files = nr_files;

	ret = resource_init_with_files(res, type);
	if (ret) {
		vaccel_error("Could not initialize resource");
		goto free;
	}

	return VACCEL_OK;

free:
	delete_files(res->files, nr_r_files);
	resource_destroy_rundir(res);
free_files:
	free(res->files);
	res->files = NULL;
	res->nr_files = 0;
release_id:
	put_resource_id(res);

	return ret;
}

int vaccel_resource_release(struct vaccel_resource *res)
{
	if (!initialized)
		return VACCEL_EPERM;

	if (!res)
		return VACCEL_EINVAL;

	if (res->id <= 0) {
		vaccel_error("Cannot release uninitialized resource");
		return VACCEL_EINVAL;
	}

	/* Check if this resource is currently registered to a session.
	 * We do not destroy currently-used resources */
	if (vaccel_resource_refcount(res)) {
		vaccel_error("Cannot release used resource %" PRId64, res->id);
		return VACCEL_EBUSY;
	}

	if (res->files) {
		delete_files(res->files, res->nr_files);
		free(res->files);
		res->files = NULL;
	}
	res->nr_files = 0;

	if (res->rundir) {
		resource_destroy_rundir(res);
		res->rundir = NULL;
	}

	if (res->paths) {
		for (size_t i = 0; i < res->nr_paths; i++) {
			if (!res->paths[i])
				continue;

			free(res->paths[i]);
		}
		free(res->paths);
		res->paths = NULL;
	}
	res->nr_paths = 0;

	list_unlink_entry(&res->entry);

	vaccel_debug("Released resource %" PRId64, res->id);

	put_resource_id(res);

	return VACCEL_OK;
}

int vaccel_resource_new(struct vaccel_resource **res, const char *path,
			vaccel_resource_t type)
{
	return vaccel_resource_multi_new(res, &path, 1, type);
}

int vaccel_resource_multi_new(struct vaccel_resource **res, const char **paths,
			      size_t nr_paths, vaccel_resource_t type)
{
	if (!res)
		return VACCEL_EINVAL;

	struct vaccel_resource *r = (struct vaccel_resource *)malloc(
		sizeof(struct vaccel_resource));
	if (!r)
		return VACCEL_ENOMEM;

	int ret = vaccel_resource_init_multi(r, paths, nr_paths, type);
	if (ret) {
		free(r);
		return ret;
	}

	*res = r;

	return VACCEL_OK;
}

int vaccel_resource_from_buf(struct vaccel_resource **res, const void *buf,
			     size_t nr_bytes, vaccel_resource_t type,
			     const char *filename)
{
	if (!res)
		return VACCEL_EINVAL;

	struct vaccel_resource *r = (struct vaccel_resource *)malloc(
		sizeof(struct vaccel_resource));
	if (!r)
		return VACCEL_ENOMEM;

	int ret =
		vaccel_resource_init_from_buf(r, buf, nr_bytes, type, filename);
	if (ret) {
		free(r);
		return ret;
	}

	*res = r;

	return VACCEL_OK;
}

int vaccel_resource_from_files(struct vaccel_resource **res,
			       const struct vaccel_file **files,
			       size_t nr_files, vaccel_resource_t type)
{
	if (!res)
		return VACCEL_EINVAL;

	struct vaccel_resource *r = (struct vaccel_resource *)malloc(
		sizeof(struct vaccel_resource));
	if (!r)
		return VACCEL_ENOMEM;

	int ret = vaccel_resource_init_from_files(r, files, nr_files, type);
	if (ret) {
		free(r);
		return ret;
	}

	*res = r;

	return VACCEL_OK;
}

int vaccel_resource_delete(struct vaccel_resource *res)
{
	int ret = vaccel_resource_release(res);
	if (ret)
		return ret;

	free(res);

	return VACCEL_OK;
}

int vaccel_resource_register(struct vaccel_resource *res,
			     struct vaccel_session *sess)
{
	int ret;

	if (!initialized)
		return VACCEL_EPERM;

	if (!res || res->path_type >= VACCEL_PATH_MAX || !sess)
		return VACCEL_EINVAL;

	switch (res->path_type) {
	case VACCEL_PATH_LOCAL_FILE:
		ret = resource_add_files_from_local(res, sess->is_virtio);
		break;
	case VACCEL_PATH_LOCAL_DIR:
		ret = resource_add_files_from_dir(res, sess->is_virtio);
		break;
	case VACCEL_PATH_REMOTE_FILE:
		ret = resource_add_files_from_remote(res, !sess->is_virtio);
		if (ret == VACCEL_ENOTSUP)
			vaccel_error(
				"Adding remote resource files requires building with libcurl");
		break;
	case VACCEL_PATH_MAX:
		vaccel_error("Invalid path type");
		ret = VACCEL_EINVAL;
		break;
	}
	if (ret) {
		vaccel_error("Failed to add resource files");
		return ret;
	}

	if (sess->is_virtio) {
		struct vaccel_plugin *virtio = get_virtio_plugin();
		if (virtio) {
			ret = virtio->info->resource_register(res, sess);
			if (res->remote_id <= 0 || ret) {
				vaccel_error(
					"session:%" PRId64
					" Could not register remote resource",
					sess->id);
				return ret;
			}
		} else {
			vaccel_error(
				"session:%" PRId64
				" Could not register remote resource: no VirtIO Plugin loaded yet",
				sess->id);
			return VACCEL_ENOTSUP;
		}
	}

	ret = session_register_resource(sess, res);
	if (ret)
		return ret;

	resource_refcount_inc(res);

	if (sess->is_virtio) {
		vaccel_debug("session:%" PRId64 " Registered resource %" PRId64
			     " with remote (id: %" PRId64 ")",
			     sess->id, res->id, res->remote_id);
	} else {
		vaccel_debug("session:%" PRId64 " Registered resource %" PRId64,
			     sess->id, res->id);
	}

	return VACCEL_OK;
}

int vaccel_resource_unregister(struct vaccel_resource *res,
			       struct vaccel_session *sess)
{
	if (!initialized)
		return VACCEL_EPERM;

	if (!res || !sess)
		return VACCEL_EINVAL;

	if (res->id <= 0) {
		vaccel_error("Cannot unregister uninitialized resource");
		return VACCEL_EINVAL;
	}

	int ret = session_unregister_resource(sess, res);
	if (ret) {
		vaccel_error("session:%" PRId64
			     " Could not unregister resource %" PRId64,
			     sess->id, res->id);
		return ret;
	}

	resource_refcount_dec(res);

	if (sess->is_virtio) {
		struct vaccel_plugin *virtio = get_virtio_plugin();
		if (virtio) {
			ret = virtio->info->resource_unregister(res, sess);
			if (ret) {
				vaccel_error(
					"session:%" PRId64
					" Could not unregister remote resource %" PRId64,
					sess->id, res->remote_id);
				return ret;
			}
		} else {
			vaccel_error(
				"session:%" PRId64
				" Could not unregister remote resource %" PRId64
				": no VirtIO Plugin loaded yet",
				sess->id, res->remote_id);
			return VACCEL_ENOTSUP;
		}
	}

	vaccel_debug("session:%" PRId64 " Unregistered resource %" PRId64,
		     sess->id, res->id);

	return VACCEL_OK;
}

int vaccel_resource_directory(struct vaccel_resource *res, char *out_path,
			      size_t out_path_size, char **alloc_path)
{
	if (!res || (!alloc_path && !out_path))
		return VACCEL_EINVAL;

	if (res->id <= 0) {
		vaccel_error(
			"Cannot return directory for uninitialized resource");
		return VACCEL_EINVAL;
	}

	if (res->path_type != VACCEL_PATH_LOCAL_DIR) {
		vaccel_error(
			"Cannot return directory for resource not created from a directory");
		return VACCEL_ENOTSUP;
	}

	const char *dir;
	if (res->rundir) {
		dir = res->rundir;
	} else {
		if (!res->nr_paths || !res->paths || !res->paths[0])
			return VACCEL_EINVAL;
		dir = res->paths[0];
	}

	if (alloc_path == NULL) {
		if (strlen(dir) >= out_path_size) {
			vaccel_error(
				"Buffer size too small for directory path");
			return VACCEL_EINVAL;
		}
		strncpy(out_path, dir, out_path_size);
	} else {
		*alloc_path = strdup(dir);
		if (!*alloc_path)
			return VACCEL_ENOMEM;
	}

	return VACCEL_OK;
}
