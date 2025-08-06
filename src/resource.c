// SPDX-License-Identifier: Apache-2.0

#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE

#include "resource.h"
#include "blob.h"
#include "core.h"
#include "error.h"
#include "id_pool.h"
#include "list.h"
#include "log.h"
#include "plugin.h"
#include "resource_registration.h"
#include "session.h"
#include "utils/fs.h"
#include "utils/net.h"
#include "utils/path.h"
#include <assert.h>
#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <linux/limits.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { VACCEL_RESOURCES_MAX = 2048 };

static struct {
	/* true if the resources component has been initialized */
	bool initialized;

	/* available resource ids */
	id_pool_t id_pool;

	/* array of per-type lists of all the created resources */
	vaccel_list_t all[VACCEL_RESOURCE_MAX];

	/* array of per-type counters for all the created resources */
	size_t count[VACCEL_RESOURCE_MAX];

	/* lock for lists/counters */
	pthread_mutex_t lock;
} resources = { .initialized = false };

int resources_bootstrap(void)
{
	int ret = id_pool_init(&resources.id_pool, VACCEL_RESOURCES_MAX);
	if (ret)
		return ret;

	for (int i = 0; i < VACCEL_RESOURCE_MAX; ++i) {
		list_init(&resources.all[i]);
		resources.count[i] = 0;
	}
	pthread_mutex_init(&resources.lock, NULL);

	resources.initialized = true;
	return VACCEL_OK;
}

int resources_cleanup(void)
{
	if (!resources.initialized)
		return VACCEL_OK;

	vaccel_debug("Cleaning up resources");

	pthread_mutex_lock(&resources.lock);

	for (int i = 0; i < VACCEL_RESOURCE_MAX; ++i) {
		struct vaccel_resource *res;
		struct vaccel_resource *tmp;
		resource_for_each_safe(res, tmp, &resources.all[i])
		{
			pthread_mutex_unlock(&resources.lock);
			vaccel_resource_release(res);
			pthread_mutex_lock(&resources.lock);
		}
	}

	pthread_mutex_unlock(&resources.lock);

	pthread_mutex_destroy(&resources.lock);
	resources.initialized = false;

	return id_pool_release(&resources.id_pool);
}

int vaccel_resource_get_by_id(struct vaccel_resource **res, vaccel_id_t id)
{
	if (!resources.initialized)
		return VACCEL_EPERM;

	if (!res)
		return VACCEL_EINVAL;

	pthread_mutex_lock(&resources.lock);

	for (int i = 0; i < VACCEL_RESOURCE_MAX; ++i) {
		struct vaccel_resource *r;
		resource_for_each(r, &resources.all[i])
		{
			if (id == r->id) {
				*res = r;
				pthread_mutex_unlock(&resources.lock);
				return VACCEL_OK;
			}
		}
	}

	pthread_mutex_unlock(&resources.lock);
	return VACCEL_ENOENT;
}

int vaccel_resource_get_by_type(struct vaccel_resource **res,
				vaccel_resource_type_t type)
{
	if (!resources.initialized)
		return VACCEL_EPERM;

	if (!res || type >= VACCEL_RESOURCE_MAX)
		return VACCEL_EINVAL;

	pthread_mutex_lock(&resources.lock);

	if (list_empty(&resources.all[type])) {
		pthread_mutex_unlock(&resources.lock);
		return VACCEL_ENOENT;
	}

	struct vaccel_resource *r = list_get_container(
		resources.all[type].next, struct vaccel_resource, entry);
	*res = r;

	pthread_mutex_unlock(&resources.lock);
	return VACCEL_OK;
}

int vaccel_resource_get_all_by_type(struct vaccel_resource ***res,
				    size_t *nr_found,
				    vaccel_resource_type_t type)
{
	if (!resources.initialized)
		return VACCEL_EPERM;

	if (!res || type >= VACCEL_RESOURCE_MAX || !nr_found)
		return VACCEL_EINVAL;

	pthread_mutex_lock(&resources.lock);
	size_t nr_res = resources.count[type];
	pthread_mutex_unlock(&resources.lock);

	if (nr_res == 0) {
		*nr_found = 0;
		return VACCEL_ENOENT;
	}

	*res = (struct vaccel_resource **)malloc(
		nr_res * sizeof(struct vaccel_resource *));
	if (!*res)
		return VACCEL_ENOMEM;

	pthread_mutex_lock(&resources.lock);

	size_t cnt = 0;
	struct vaccel_resource *r;
	resource_for_each(r, &resources.all[type])
	{
		if (cnt < nr_res)
			(*res)[cnt++] = r;
		else
			break;
	}
	*nr_found = cnt;

	pthread_mutex_unlock(&resources.lock);
	return VACCEL_OK;
}

static void get_resource_id(struct vaccel_resource *res)
{
	res->id = id_pool_get(&resources.id_pool);
}

static void put_resource_id(struct vaccel_resource *res)
{
	if (id_pool_put(&resources.id_pool, res->id))
		vaccel_warn("Could not return resource ID to pool");
	res->id = 0;
}

void resource_refcount_inc(struct vaccel_resource *res)
{
	if (!res) {
		vaccel_error("Refcounting invalid resource");
		return;
	}

	atomic_fetch_add(&res->refcount, 1);
}

void resource_refcount_dec(struct vaccel_resource *res)
{
	if (!res) {
		vaccel_error("Refcounting invalid resource");
		return;
	}

	atomic_fetch_sub(&res->refcount, 1);
}

long int vaccel_resource_refcount(const struct vaccel_resource *res)
{
	if (!res) {
		vaccel_error("Refcounting invalid resource");
		return -VACCEL_EINVAL;
	}

	return atomic_load(&res->refcount);
}

int resource_create_rundir(struct vaccel_resource *res)
{
	if (!res || res->id <= 0) {
		vaccel_error("Trying to create rundir for invalid resource");
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

static void delete_blobs(struct vaccel_blob **blobs, size_t nr_blobs)
{
	if (!blobs)
		return;

	for (size_t i = 0; i < nr_blobs; i++) {
		if (!blobs[i])
			continue;

		int ret = vaccel_blob_delete(blobs[i]);
		if (ret) {
			vaccel_warn("Could not delete blob %zu", i);
			continue;
		}
		blobs[i] = NULL;
	}
}

static int resource_populate_blob_data(struct vaccel_resource *res)
{
	if (!res || !res->blobs || !res->nr_blobs)
		return VACCEL_EINVAL;

	for (size_t i = 0; i < res->nr_blobs; i++) {
		if (!res->blobs[i])
			return VACCEL_EINVAL;

		int ret = vaccel_blob_read(res->blobs[i]);
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

	int ret = vaccel_blob_new(&res->blobs[idx], path);
	if (ret)
		vaccel_error("Could not create vaccel_blob for %s", path);

	return ret;
}

static int resource_add_blobs_from_dir(struct vaccel_resource *res,
				       bool with_data)
{
	if (!res)
		return VACCEL_EINVAL;

	/* If files exist only populate the file data if chosen */
	if (res->blobs && res->nr_blobs) {
		if (with_data)
			return resource_populate_blob_data(res);
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

	res->blobs = (struct vaccel_blob **)malloc(
		nr_dirfiles * sizeof(struct vaccel_blob *));
	if (!res->blobs)
		return VACCEL_ENOMEM;

	for (int i = 0; i < nr_dirfiles; i++)
		res->blobs[i] = NULL;

	/* Create vaccel_blob struct for file paths. If the files are in
	 * subdirectories they will be persisted in a flat directory remotely */
	int ret = fs_dir_process_files(res->paths[0], add_dir_files_callback,
				       res, nr_dirfiles, NULL);
	if (ret < 0) {
		vaccel_error("Could not process files from dir %s",
			     res->paths[0]);
		goto free;
	}

	assert(nr_dirfiles == ret);
	res->nr_blobs = nr_dirfiles;

	if (with_data) {
		ret = resource_populate_blob_data(res);
		if (ret) {
			vaccel_error("Could not populate resource blob data");
			goto free;
		}
	}

	return VACCEL_OK;

free:
	delete_blobs(res->blobs, res->nr_blobs);
	free(res->blobs);
	res->blobs = NULL;
	res->nr_blobs = 0;

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

static int resource_add_blobs(struct vaccel_resource *res, bool remote,
			      bool download)
{
	if (!res)
		return VACCEL_EINVAL;

	if (res->blobs && res->nr_blobs)
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

	res->blobs = (struct vaccel_blob **)malloc(
		res->nr_paths * sizeof(struct vaccel_blob *));
	if (!res->blobs)
		return VACCEL_ENOMEM;

	for (size_t i = 0; i < res->nr_paths; i++)
		res->blobs[i] = NULL;

	size_t nr_blobs = 0;
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

			ret = vaccel_blob_new(&res->blobs[i], path);
			if (ret) {
				vaccel_error(
					"Could not create vaccel_blob for %s",
					path);
				goto free;
			}
			/* Mark the path as owned so it will be deleted on release */
			res->blobs[i]->path_owned = true;
		} else {
			ret = vaccel_blob_new(&res->blobs[i], res->paths[i]);
			if (ret) {
				vaccel_error(
					"Could not create vaccel_blob for %s",
					res->paths[i]);
				goto free;
			}
		}

		++nr_blobs;
	}
	assert(res->nr_paths == nr_blobs);
	res->nr_blobs = nr_blobs;

	return VACCEL_OK;

free:
	delete_blobs(res->blobs, nr_blobs);
	free(res->blobs);
	res->blobs = NULL;
	res->nr_blobs = 0;
cleanup_rundir:
	resource_destroy_rundir(res);

	return ret;
}

static int resource_add_blobs_from_local(struct vaccel_resource *res,
					 bool with_data)
{
	if (!res)
		return VACCEL_EINVAL;

	int ret = resource_add_blobs(res, false, false);
	if (ret)
		return ret;

	if (with_data)
		return resource_populate_blob_data(res);

	return VACCEL_OK;
}

static int resource_add_blobs_from_remote(struct vaccel_resource *res,
					  bool download)
{
	if (!res)
		return VACCEL_EINVAL;

	return resource_add_blobs(res, true, download);
}

static int resource_init_with_paths(struct vaccel_resource *res,
				    vaccel_resource_type_t type)
{
	if (!res || !res->paths || !res->nr_paths || !res->paths[0] ||
	    type >= VACCEL_RESOURCE_MAX)
		return VACCEL_EINVAL;

	res->remote_id = 0;
	res->type = type;
	res->blobs = NULL;
	res->nr_blobs = 0;
	res->rundir = NULL;

	list_init(&res->sessions);
	pthread_mutex_init(&res->sessions_lock, NULL);
	atomic_init(&res->refcount, 0);

	pthread_mutex_lock(&resources.lock);
	list_add_tail(&resources.all[res->type], &res->entry);
	resources.count[res->type]++;
	pthread_mutex_unlock(&resources.lock);

	vaccel_debug("Initialized resource %" PRId64, res->id);

	return VACCEL_OK;
}

static int resource_init_with_blobs(struct vaccel_resource *res,
				    vaccel_resource_type_t type)
{
	if (!res || !res->blobs || !res->nr_blobs ||
	    type >= VACCEL_RESOURCE_MAX || res->id <= 0)
		return VACCEL_EINVAL;

	res->remote_id = 0;
	res->type = type;
	res->path_type = VACCEL_PATH_LOCAL_FILE;
	res->paths = NULL;
	res->nr_paths = 0;

	list_init(&res->sessions);
	pthread_mutex_init(&res->sessions_lock, NULL);
	atomic_init(&res->refcount, 0);

	pthread_mutex_lock(&resources.lock);
	list_add_tail(&resources.all[res->type], &res->entry);
	resources.count[res->type]++;
	pthread_mutex_unlock(&resources.lock);

	vaccel_debug("Initialized resource %" PRId64, res->id);

	return VACCEL_OK;
}

int vaccel_resource_init(struct vaccel_resource *res, const char *path,
			 vaccel_resource_type_t type)
{
	return vaccel_resource_init_multi(res, &path, 1, type);
}

int vaccel_resource_init_multi(struct vaccel_resource *res, const char **paths,
			       size_t nr_paths, vaccel_resource_type_t type)
{
	int ret;

	if (!resources.initialized)
		return VACCEL_EPERM;

	if (!res || !paths || !nr_paths || type >= VACCEL_RESOURCE_MAX)
		return VACCEL_EINVAL;

	get_resource_id(res);
	if (res->id < 0)
		return -(int)res->id;

	res->plugin_priv = NULL;
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

		vaccel_path_type_t type;
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
				  size_t nr_bytes, vaccel_resource_type_t type,
				  const char *filename, bool mem_only)
{
	int ret;

	if (!resources.initialized)
		return VACCEL_EPERM;

	if (!res || !buf || !nr_bytes || type >= VACCEL_RESOURCE_MAX)
		return VACCEL_EINVAL;

	get_resource_id(res);
	if (res->id < 0)
		return -(int)res->id;

	res->plugin_priv = NULL;
	res->blobs =
		(struct vaccel_blob **)malloc(sizeof(struct vaccel_blob *));
	if (!res->blobs) {
		ret = VACCEL_ENOMEM;
		goto release_id;
	}

	if (!mem_only) {
		ret = resource_create_rundir(res);
		if (ret)
			goto free_blobs;
	} else {
		res->rundir = NULL;
	}

	bool rand = (filename == NULL);
	const char *name = rand ? "file" : filename;
	ret = vaccel_blob_from_buf(&res->blobs[0], buf, nr_bytes, false, name,
				   res->rundir, rand);
	if (ret) {
		vaccel_error("Could not create vaccel_blob from buffer");
		goto cleanup_rundir;
	}
	res->nr_blobs = 1;

	ret = resource_init_with_blobs(res, type);
	if (ret) {
		vaccel_error("Could not initialize resource");
		goto delete_blob;
	}

	return VACCEL_OK;

delete_blob:
	delete_blobs(res->blobs, res->nr_blobs);
cleanup_rundir:
	if (res->rundir)
		resource_destroy_rundir(res);
free_blobs:
	free(res->blobs);
	res->blobs = NULL;
	res->nr_blobs = 0;
release_id:
	put_resource_id(res);

	return ret;
}

int vaccel_resource_init_from_blobs(struct vaccel_resource *res,
				    const struct vaccel_blob **blobs,
				    size_t nr_blobs,
				    vaccel_resource_type_t type)
{
	int ret;

	if (!resources.initialized)
		return VACCEL_EPERM;

	if (!res || !blobs || !nr_blobs || type >= VACCEL_RESOURCE_MAX)
		return VACCEL_EINVAL;

	/* Confirm input blobs are valid */
	ret = VACCEL_EINVAL;
	for (size_t i = 0; i != nr_blobs; i++) {
		if (!vaccel_blob_valid(blobs[i]))
			goto empty_attributes;
	}

	get_resource_id(res);
	if (res->id < 0)
		return -(int)res->id;

	res->plugin_priv = NULL;
	res->blobs = (struct vaccel_blob **)malloc(
		nr_blobs * sizeof(struct vaccel_blob *));
	if (!res->blobs) {
		ret = VACCEL_ENOMEM;
		goto release_id;
	}

	for (size_t i = 0; i != nr_blobs; i++)
		res->blobs[i] = NULL;

	/* Check if rundir should be created */
	bool create_rundir = false;
	for (size_t i = 0; i != nr_blobs; i++) {
		if (blobs[i]->type != VACCEL_BLOB_BUFFER) {
			create_rundir = true;
			break;
		}
	}

	if (create_rundir) {
		ret = resource_create_rundir(res);
		if (ret)
			goto free_blobs;
	} else {
		res->rundir = NULL;
	}

	bool randomize = false;
	size_t nr_r_blobs = 0;
	for (size_t i = 0; i < nr_blobs; i++) {
		if (blobs[i]->type == VACCEL_BLOB_FILE) {
			ret = vaccel_blob_new(&res->blobs[i], blobs[i]->path);
		} else {
			const char *dir = blobs[i]->type == VACCEL_BLOB_BUFFER ?
						  NULL :
						  res->rundir;
			ret = vaccel_blob_from_buf(
				&res->blobs[i], blobs[i]->data, blobs[i]->size,
				blobs[i]->data_owned, blobs[i]->name, dir,
				randomize);
		}

		if (ret) {
			vaccel_error(
				"Could not create vaccel_blob from buffer");
			goto free;
		}

		++nr_r_blobs;
	}
	assert(nr_r_blobs == nr_blobs);
	res->nr_blobs = nr_blobs;

	ret = resource_init_with_blobs(res, type);
	if (ret) {
		vaccel_error("Could not initialize resource");
		goto free;
	}

	return VACCEL_OK;

free:
	delete_blobs(res->blobs, nr_r_blobs);
	if (create_rundir)
		resource_destroy_rundir(res);
free_blobs:
	free(res->blobs);
release_id:
	put_resource_id(res);
empty_attributes:
	res->blobs = NULL;
	res->nr_blobs = 0;

	return ret;
}

int vaccel_resource_release(struct vaccel_resource *res)
{
	if (!resources.initialized)
		return VACCEL_EPERM;

	if (!res)
		return VACCEL_EINVAL;

	if (res->id <= 0) {
		vaccel_error("Cannot release uninitialized resource");
		return VACCEL_EINVAL;
	}

	int ret = resource_registration_foreach_session(
		res, vaccel_resource_unregister);
	if (ret)
		return ret;

	pthread_mutex_destroy(&res->sessions_lock);

	if (res->blobs) {
		delete_blobs(res->blobs, res->nr_blobs);
		free(res->blobs);
		res->blobs = NULL;
	}
	res->nr_blobs = 0;

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
	res->plugin_priv = NULL;

	pthread_mutex_lock(&resources.lock);
	list_unlink_entry(&res->entry);
	resources.count[res->type]--;
	pthread_mutex_unlock(&resources.lock);

	vaccel_debug("Released resource %" PRId64, res->id);

	put_resource_id(res);

	return VACCEL_OK;
}

int vaccel_resource_new(struct vaccel_resource **res, const char *path,
			vaccel_resource_type_t type)
{
	return vaccel_resource_multi_new(res, &path, 1, type);
}

int vaccel_resource_multi_new(struct vaccel_resource **res, const char **paths,
			      size_t nr_paths, vaccel_resource_type_t type)
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
			     size_t nr_bytes, vaccel_resource_type_t type,
			     const char *filename, bool mem_only)
{
	if (!res)
		return VACCEL_EINVAL;

	struct vaccel_resource *r = (struct vaccel_resource *)malloc(
		sizeof(struct vaccel_resource));
	if (!r)
		return VACCEL_ENOMEM;

	int ret = vaccel_resource_init_from_buf(r, buf, nr_bytes, type,
						filename, mem_only);
	if (ret) {
		free(r);
		return ret;
	}

	*res = r;

	return VACCEL_OK;
}

int vaccel_resource_from_blobs(struct vaccel_resource **res,
			       const struct vaccel_blob **blobs,
			       size_t nr_blobs, vaccel_resource_type_t type)
{
	if (!res)
		return VACCEL_EINVAL;

	struct vaccel_resource *r = (struct vaccel_resource *)malloc(
		sizeof(struct vaccel_resource));
	if (!r)
		return VACCEL_ENOMEM;

	int ret = vaccel_resource_init_from_blobs(r, blobs, nr_blobs, type);
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

	if (!resources.initialized)
		return VACCEL_EPERM;

	if (!res || res->path_type >= VACCEL_PATH_MAX || !sess)
		return VACCEL_EINVAL;

	switch (res->path_type) {
	case VACCEL_PATH_LOCAL_FILE:
		ret = resource_add_blobs_from_local(res, sess->is_virtio);
		break;
	case VACCEL_PATH_LOCAL_DIR:
		ret = resource_add_blobs_from_dir(res, sess->is_virtio);
		break;
	case VACCEL_PATH_REMOTE_FILE:
		ret = resource_add_blobs_from_remote(res, !sess->is_virtio);
		if (ret == VACCEL_ENOTSUP)
			vaccel_error(
				"Adding remote resource blobs requires building with libcurl");
		break;
	case VACCEL_PATH_MAX:
		vaccel_error("Invalid path type");
		ret = VACCEL_EINVAL;
		break;
	}
	if (ret) {
		vaccel_error("Failed to add resource blobs");
		return ret;
	}

	if (sess->is_virtio) {
		if (sess->plugin->info->is_virtio) {
			ret = sess->plugin->info->resource_register(res, sess);
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

	if (resource_registration_find(res, sess)) {
		vaccel_warn("session:%" PRId64 " Resource %" PRId64
			    " already registered",
			    sess->id, res->id);
		return VACCEL_OK;
	}

	struct resource_registration *reg;
	ret = resource_registration_new(&reg, res, sess);
	if (ret)
		return ret;

	ret = resource_registration_link(reg);
	if (ret) {
		resource_registration_delete(reg);
		return ret;
	}

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
	if (!resources.initialized)
		return VACCEL_EPERM;

	if (!res || !sess)
		return VACCEL_EINVAL;

	if (res->id <= 0) {
		vaccel_error("Cannot unregister uninitialized resource");
		return VACCEL_EINVAL;
	}

	struct resource_registration *reg =
		resource_registration_find_and_unlink(res, sess);
	if (!reg) {
		vaccel_error("session:%" PRId64 " Resource %" PRId64
			     " not registered",
			     sess->id, res->id);
		return VACCEL_EINVAL;
	}

	int ret = resource_registration_delete(reg);
	if (ret) {
		vaccel_error("session:%" PRId64
			     " Could not delete resource %" PRId64
			     " registration",
			     sess->id, res->id);
		return ret;
	}

	if (sess->is_virtio) {
		if (sess->plugin->info->is_virtio) {
			ret = sess->plugin->info->resource_unregister(res,
								      sess);
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

int vaccel_resource_sync(struct vaccel_resource *res,
			 struct vaccel_session *sess)
{
	int ret;

	if (!resources.initialized)
		return VACCEL_EPERM;

	if (!res || !sess)
		return VACCEL_EINVAL;

	if (!vaccel_session_has_resource(sess, res)) {
		vaccel_error("Resource %" PRId64
			     " is not registered to session %" PRId64 "",
			     res->id, sess->id);
		return VACCEL_EPERM;
	}

	if (res->blobs[0]->type != VACCEL_BLOB_BUFFER) {
		vaccel_error("Only buffer resources can be synced");
		return VACCEL_EINVAL;
	}

	if (!sess->is_virtio) {
		vaccel_warn("Non-VirtIO session, no need to sync");
		return VACCEL_OK;
	}

	if (sess->plugin->info->is_virtio) {
		ret = sess->plugin->info->resource_sync(res, sess);
		if (ret != VACCEL_OK) {
			vaccel_error("session:%" PRId64
				     " Could not synchronize resource %" PRId64,
				     sess->id, res->id);
			return ret;
		}
	} else {
		vaccel_error(
			"session:%" PRId64
			" Could not synchronize remote resource: no VirtIO Plugin loaded yet",
			sess->id);
		return VACCEL_ENOTSUP;
	}

	return VACCEL_OK;
}
