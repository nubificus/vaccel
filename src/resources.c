// SPDX-License-Identifier: Apache-2.0

#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE

#include "resources.h"
#include "error.h"
#include "id_pool.h"
#include "list.h"
#include "log.h"
#include "plugin.h"
#include "utils.h"
#include "vaccel.h"

#include <dirent.h>
#include <libgen.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <vaccel_file.h>

enum { MAX_RESOURCES = 2048, MAX_RESOURCE_RUNDIR = 1024, MAX_FILELEN = 1024 };

static bool initialized = false;
static id_pool_t id_pool;

/* All the live (created) vAccel resources.
 * At the moment, this is an array where each element is a list of all
 * resources of the same time. We should think the data structure again.
 */
static list_t live_resources[VACCEL_RESOURCE_MAX];

int resources_bootstrap(void)
{
	int ret = id_pool_new(&id_pool, MAX_RESOURCES);
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
			vaccel_resource_destroy(res);
	}

	initialized = false;

	return id_pool_destroy(&id_pool);
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

int resource_set_deps(struct vaccel_resource *res,
		      struct vaccel_resource **deps, size_t nr_deps)
{
	if (!res || !deps || !nr_deps)
		return VACCEL_EINVAL;

	struct vaccel_plugin *virtio = get_virtio_plugin();
	if (virtio) {
		int err = virtio->info->resource_set_deps(res, deps, nr_deps);
		if (err)
			return err;
	}

	res->deps = deps;
	res->nr_deps = nr_deps;

	return VACCEL_OK;
}

int vaccel_resource_get_deps(struct vaccel_resource ***deps, size_t *nr_deps,
			     struct vaccel_resource *res)
{
	if (!deps || !nr_deps || !res)
		return VACCEL_EINVAL;

	*deps = res->deps;
	*nr_deps = res->nr_deps;

	return VACCEL_OK;
}

int vaccel_resource_deps_to_ids(vaccel_id_t *ids, struct vaccel_resource **deps,
				size_t nr_deps)
{
	if (!ids || !deps || !nr_deps)
		return VACCEL_EINVAL;

	for (size_t i = 0; i < nr_deps; i++) {
		ids[i] = deps[i]->id;
	}

	return VACCEL_OK;
}

int vaccel_resource_deps_from_ids(struct vaccel_resource **deps,
				  vaccel_id_t *ids, size_t nr_ids)
{
	if (!deps || !ids || !nr_ids)
		return VACCEL_EINVAL;

	for (size_t i = 0; i < nr_ids; i++) {
		struct vaccel_resource *res;
		int ret = vaccel_resource_get_by_id(&res, ids[i]);
		if (ret)
			return VACCEL_EINVAL;
		deps[i] = res;
	}

	return VACCEL_OK;
}

int vaccel_resource_set_deps_from_ids(struct vaccel_resource *res,
				      vaccel_id_t *ids, size_t nr_ids)
{
	if (!res || !ids || !nr_ids)
		return VACCEL_EINVAL;

	struct vaccel_resource **deps =
		(struct vaccel_resource **)malloc(sizeof(*deps) * nr_ids);
	if (!deps)
		return VACCEL_ENOMEM;

	int ret = vaccel_resource_deps_from_ids(deps, ids, nr_ids);
	if (ret) {
		free(deps);
		return VACCEL_ENOMEM;
	}

	res->deps = deps;
	res->nr_deps = nr_ids;

	return VACCEL_OK;
}

int resource_unset_deps(struct vaccel_resource *res)
{
	if (!res)
		return VACCEL_EINVAL;

	res->deps = NULL;
	res->nr_deps = 0;

	return VACCEL_OK;
}

int vaccel_resource_destroy(struct vaccel_resource *res)
{
	if (!initialized)
		return VACCEL_EPERM;

	if (!res)
		return VACCEL_EINVAL;

	/* Check if this resource is currently registered to a session.
	 * We do not destroy currently-used resources */
	if (atomic_load(&res->refcount)) {
		vaccel_warn("Cannot destroy used resource %lld", res->id);
		return VACCEL_EBUSY;
	}

	if (res->files) {
		for (size_t i = 0; i < res->nr_files; i++) {
			if (!res->files[i])
				continue;

			vaccel_file_destroy(res->files[i]);
			free(res->files[i]);
		}
		free(res->files);
	}
	res->nr_files = 0;

	id_pool_release(&id_pool, res->id);

	list_unlink_entry(&res->entry);

	if (res->rundir) {
		cleanup_rundir(res->rundir);
		free(res->rundir);
	}

	if (res->paths) {
		for (size_t i = 0; i < res->nr_paths; i++) {
			if (!res->paths[i])
				continue;

			free(res->paths[i]);
		}
		free(res->paths);
	}
	res->nr_paths = 0;

	if (res->deps || res->nr_deps)
		vaccel_warn("Resource %lld has deps that will not be destroyed",
			    res->id);

	return VACCEL_OK;
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

int resource_create_rundir(struct vaccel_resource *res)
{
	if (!res) {
		vaccel_error(
			"BUG! Trying to create rundir for invalid resource");
		return VACCEL_EINVAL;
	}

	const char *root_rundir = vaccel_rundir();

	char rundir[MAX_RESOURCE_RUNDIR];
	int len = snprintf(rundir, MAX_RESOURCE_RUNDIR, "%s/resource.%lld",
			   root_rundir, res->id);

	if (len == MAX_RESOURCE_RUNDIR) {
		vaccel_error("rundir path '%s/resource.%lld' too long",
			     root_rundir, res->id);
		return VACCEL_ENAMETOOLONG;
	}

	int ret = mkdir(rundir, 0700);
	if (ret)
		return errno;

	res->rundir = strndup(rundir, MAX_RESOURCE_RUNDIR);
	if (!res->rundir)
		return VACCEL_ENOMEM;

	return VACCEL_OK;
}

static int is_dir(const char *path)
{
	struct stat path_stat;
	stat(path, &path_stat);
	return S_ISDIR(path_stat.st_mode);
}

static int is_file(const char *path)
{
	struct stat path_stat;
	stat(path, &path_stat);
	return S_ISREG(path_stat.st_mode);
}

static int is_url(const char *path)
{
	const char *prefix = "http://";
	if (strncmp(path, prefix, strlen(prefix)) == 0)
		return 1;
	return 0;
}

static int get_num_files(const char *path)
{
	DIR *d;
	struct dirent *dir;
	int nr_files = 0;

	// FIXME: print errors
	if (!path)
		return VACCEL_EINVAL;

	d = opendir(path);
	if (!d)
		return VACCEL_EINVAL;

	while ((dir = readdir(d)) != NULL) {
		if (dir->d_type == DT_REG)
			++nr_files;
	}

	closedir(d);

	return nr_files;
}

static int resource_add_files_from_dir(struct vaccel_resource *res)
{
	DIR *d;
	struct dirent *dir;
	int ret;
	struct vaccel_file **files;
	size_t nr_files;
	char filepath[MAX_FILELEN] = { 0 };

	// FIXME: print errors
	if (!res || !res->paths || res->nr_paths != 1)
		return VACCEL_EINVAL;

	nr_files = get_num_files(res->paths[0]);
	if (!nr_files)
		return VACCEL_EINVAL;

	files = (struct vaccel_file **)malloc(nr_files *
					      sizeof(struct vaccel_file *));
	if (!files)
		return VACCEL_ENOMEM;

	d = opendir(res->paths[0]);
	if (!d)
		return VACCEL_EINVAL;

	for (size_t i = 0; i < nr_files; i++)
		files[i] = NULL;

	size_t pathlen = strlen(res->paths[0]) + 1;
	if (pathlen >= MAX_FILELEN)
		return VACCEL_ENAMETOOLONG;

	memcpy(filepath, res->paths[0], pathlen);
	size_t baselen = strlen(filepath) + 1;
	filepath[baselen - 1] = '/';
	size_t i = 0;
	while ((dir = readdir(d)) != NULL) {
		if (i > nr_files) {
			vaccel_error(
				"Computed and actual file numbers differ (%zu vs %zu)",
				res->nr_paths, i);
			ret = EINVAL;
			goto free;
		}
		if (dir->d_type == DT_REG) {
			size_t filenamelen = strlen(dir->d_name) + 1;
			if (filenamelen + baselen >= MAX_FILELEN) {
				ret = VACCEL_ENAMETOOLONG;
				goto free;
			}

			memcpy(&filepath[baselen], dir->d_name, filenamelen);

			files[i] = (struct vaccel_file *)malloc(
				sizeof(struct vaccel_file));
			if (!files[i]) {
				ret = VACCEL_ENOMEM;
				goto free;
			}

			ret = vaccel_file_new(files[i++], filepath);
			if (ret)
				goto free;
		}
		memset(&filepath[baselen], 0, MAX_FILELEN - baselen);
	}

	closedir(d);

	res->files = files;
	res->nr_files = nr_files;

	return VACCEL_OK;

free:
	for (size_t i = 0; i < res->nr_files; i++) {
		if (!res->files[i])
			continue;

		vaccel_file_destroy(res->files[i]);
		free(res->files[i]);
	}
	free(res->files);
	closedir(d);

	return ret;
}

static int resource_add_files_from_local(struct vaccel_resource *res)
{
	int ret;
	struct vaccel_file **files;

	if (!res || !res->paths || !res->nr_paths)
		return VACCEL_EINVAL;

	files = (struct vaccel_file **)malloc(res->nr_paths *
					      sizeof(struct vaccel_file *));
	if (!files)
		return VACCEL_ENOMEM;

	for (size_t i = 0; i < res->nr_paths; i++)
		files[i] = NULL;

	for (size_t i = 0; i < res->nr_paths; i++) {
		if (!res->paths[i]) {
			ret = VACCEL_EINVAL;
			goto free;
		}

		files[i] = (struct vaccel_file *)malloc(
			sizeof(struct vaccel_file));
		if (!files[i]) {
			ret = VACCEL_ENOMEM;
			goto free;
		}

		ret = vaccel_file_new(files[i], res->paths[i]);
		if (ret) {
			ret = VACCEL_EINVAL;
			goto free;
		}
	}

	res->files = files;
	res->nr_files = res->nr_paths;

	return VACCEL_OK;

free:
	for (size_t i = 0; i < res->nr_paths; i++) {
		if (!files[i])
			continue;

		vaccel_file_destroy(files[i]);
		free(files[i]);
	}
	free(files);

	return ret;
}

int vaccel_resource_new(struct vaccel_resource *res, char *path,
			vaccel_resource_t type)
{
	int ret;

	if (!initialized)
		return VACCEL_EPERM;

	if (!res || type >= VACCEL_RESOURCE_MAX || !path)
		return VACCEL_EINVAL;

	res->id = id_pool_get(&id_pool);
	if (res->id <= 0)
		return VACCEL_EUSERS;

	res->paths = (char **)malloc(sizeof(char *));
	if (!res->paths) {
		ret = VACCEL_ENOMEM;
		goto release_id;
	}

	res->paths[0] = strdup(path);
	if (!res->paths[0]) {
		ret = VACCEL_ENOMEM;
		goto free;
	}
	res->nr_paths = 1;

	if (is_url(path)) {
		res->path_type = VACCEL_PATH_REMOTE;
	} else if (is_dir(path)) {
		res->path_type = VACCEL_PATH_DIR;
	} else if (is_file(path)) {
		res->path_type = VACCEL_PATH_LOCAL;
	} else {
		ret = VACCEL_EINVAL;
		goto free;
	}

	res->remote_id = -1;
	res->type = type;
	res->files = NULL;
	res->nr_files = 0;
	res->deps = NULL;
	res->nr_deps = 0;
	res->rundir = NULL;
	atomic_init(&res->refcount, 0);

	list_init_entry(&res->entry);
	list_add_tail(&live_resources[0], &res->entry);

	return VACCEL_OK;

free:
	free(res->paths);
release_id:
	id_pool_release(&id_pool, res->id);

	return ret;
}

int vaccel_resource_new_from_buf(struct vaccel_resource *res, void *buf,
				 size_t nr_bytes, vaccel_resource_t type)
{
	int ret;

	if (!initialized)
		return VACCEL_EPERM;

	if (!res || type >= VACCEL_RESOURCE_MAX || !buf || !nr_bytes)
		return VACCEL_EINVAL;

	res->id = id_pool_get(&id_pool);
	if (res->id <= 0)
		return VACCEL_EUSERS;

	res->files =
		(struct vaccel_file **)malloc(sizeof(struct vaccel_file *));
	if (!res->files) {
		ret = VACCEL_ENOMEM;
		goto release_id;
	}

	res->files[0] =
		(struct vaccel_file *)malloc(sizeof(struct vaccel_file));
	if (!res->files[0]) {
		ret = VACCEL_ENOMEM;
		goto free_files;
	}

	ret = vaccel_file_from_buffer(res->files[0], buf, nr_bytes, NULL, NULL,
				      false, false);
	if (ret)
		goto free;

	ret = resource_create_rundir(res);
	if (ret)
		goto destroy_file;

	vaccel_debug("New rundir for resource %s", res->rundir);

	ret = vaccel_file_persist(res->files[0], res->rundir, "lib.so", true);
	if (ret)
		goto cleanup_rundir;

	res->remote_id = -1;
	res->type = type;
	res->path_type = VACCEL_PATH_LOCAL;
	res->paths = NULL;
	res->nr_paths = 0;
	res->nr_files = 1;
	res->deps = NULL;
	res->nr_deps = 0;
	atomic_init(&res->refcount, 0);

	list_init_entry(&res->entry);
	list_add_tail(&live_resources[0], &res->entry);

	return VACCEL_OK;

cleanup_rundir:
	cleanup_rundir(res->rundir);
	free(res->rundir);
destroy_file:
	vaccel_file_destroy(res->files[0]);
free:
	free(res->files[0]);
free_files:
	free(res->files);
release_id:
	id_pool_release(&id_pool, res->id);

	return ret;
}

int vaccel_resource_new_multi(struct vaccel_resource *res, char **paths,
			      vaccel_resource_t type, size_t nr_paths)
{
	int ret;

	if (!initialized)
		return VACCEL_EPERM;

	if (!res || type >= VACCEL_RESOURCE_MAX || !paths || !nr_paths)
		return VACCEL_EINVAL;

	res->id = id_pool_get(&id_pool);
	if (res->id <= 0)
		return VACCEL_EUSERS;

	res->paths = (char **)malloc(nr_paths * sizeof(char *));
	if (!res->paths) {
		ret = VACCEL_ENOMEM;
		goto release_id;
	}

	for (size_t i = 0; i < nr_paths; i++)
		res->paths[i] = NULL;

	for (size_t i = 0; i < nr_paths; i++) {
		res->paths[i] = strdup(paths[i]);
		if (!res->paths[i]) {
			ret = VACCEL_ENOMEM;
			goto free;
		}
	}
	res->nr_paths = nr_paths;

	res->remote_id = -1;
	res->type = type;
	res->path_type = VACCEL_PATH_LIST;
	res->files = NULL;
	res->nr_files = 0;
	res->deps = NULL;
	res->nr_deps = 0;
	res->rundir = NULL;
	atomic_init(&res->refcount, 0);

	list_init_entry(&res->entry);
	list_add_tail(&live_resources[0], &res->entry);

	return VACCEL_OK;

free:
	for (size_t i = 0; i < nr_paths; i++) {
		if (!res->paths[i])
			continue;

		free(res->paths[i]);
	}
	free(res->paths);
release_id:
	id_pool_release(&id_pool, res->id);

	return ret;
}

int vaccel_resource_register(struct vaccel_resource *res,
			     struct vaccel_session *sess)
{
	int ret;

	if (!initialized)
		return VACCEL_EPERM;

	if (!res || res->path_type >= VACCEL_PATH_MAX || !sess)
		return VACCEL_EINVAL;

	if (!res->nr_files) {
		switch (res->path_type) {
		case VACCEL_PATH_LOCAL:
		case VACCEL_PATH_LIST:
			ret = resource_add_files_from_local(res);
			break;
		case VACCEL_PATH_DIR:
			ret = resource_add_files_from_dir(res);
			break;
		case VACCEL_PATH_REMOTE:
			// TODO
			vaccel_error("Resource from URL is not implemented");
			ret = VACCEL_ENOTSUP;
			break;
		case VACCEL_PATH_MAX:
			vaccel_error("Invalid path type");
			ret = VACCEL_EINVAL;
			break;
		}
		if (ret)
			return ret;
	}

	ret = vaccel_session_register_resource(sess, res);
	if (ret)
		return ret;

	return VACCEL_OK;
}

int vaccel_resource_get_path_by_name(struct vaccel_resource *res,
				     const char *name, char *dest)
{
	if (!initialized)
		return VACCEL_EPERM;

	if (!res || !name || !dest)
		return VACCEL_EINVAL;

	for (size_t i = 0; i < res->nr_files; i++) {
		if (strcmp(basename(res->files[i]->path), name) == 0) {
			strcpy(dest, res->files[i]->path);
			return VACCEL_OK;
		}
	}
	return VACCEL_ENOENT;
}

char *vaccel_resource_get_path_by_index(struct vaccel_resource *res, size_t idx)
{
	if (!initialized || !res || !res->nr_files || !res->files)
		return NULL;

	if (idx >= res->nr_files || !res->files[idx] || !res->files[idx]->path)
		return NULL;

	return strdup(res->files[idx]->path);
}
