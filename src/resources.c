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

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <libgen.h>
#include <dirent.h>
#include <vaccel_file.h>

enum { MAX_RESOURCES = 2048, MAX_RESOURCE_RUNDIR = 1024, MAX_FILELEN = 1024 };

static bool initialized = false;
static id_pool_t id_pool;

/* All the live (created) vAccel resources.
 * At the moment, this is an array where each element is a list of all
 * resources of the same time. We should think the data structure again.
 */
static list_t live_resources[VACCEL_RES_MAX];

int resources_bootstrap(void)
{
	int ret = id_pool_new(&id_pool, MAX_RESOURCES);
	if (ret)
		return ret;

	for (int i = 0; i < VACCEL_RES_MAX; ++i)
		list_init(&live_resources[i]);

	initialized = true;

	return VACCEL_OK;
}

int resources_cleanup(void)
{
	if (!initialized)
		return VACCEL_OK;

	for (int i = 0; i < VACCEL_RES_MAX; ++i) {
		struct vaccel_resource *res;
		struct vaccel_resource *tmp;
		for_each_vaccel_resource_safe(res, tmp, &live_resources[i])
			vaccel_resource_destroy(res);
	}

	initialized = false;

	return id_pool_destroy(&id_pool);
}

int resource_get_by_id(struct vaccel_resource **resource, vaccel_id_t id)
{
	if (!initialized)
		return VACCEL_EPERM;

	for (int i = 0; i < VACCEL_RES_MAX; ++i) {
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
		int ret = resource_get_by_id(&res, ids[i]);
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

	for (size_t i = 0; i < res->nr_files; i++) {
		vaccel_file_destroy(res->files[i]);
		free(res->files[i]);
	}
	free(res->files);
	res->nr_files = 0;

	/* Check if this resources is currently registered to a session.
	 * We do not destroy currently-used resources */
	if (atomic_load(&res->refcount)) {
		vaccel_warn("Cannot destroy used resource %lld", res->id);
		return VACCEL_EBUSY;
	}

	id_pool_release(&id_pool, res->id);

	list_unlink_entry(&res->entry);

	if (res->rundir) {
		cleanup_rundir(res->rundir);
		free(res->rundir);
	}

	if (res->path)
		free(res->path);

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
	d = opendir(path);
	if (d) {
		while ((dir = readdir(d)) != NULL) {
			if (dir->d_type == DT_REG)
				++nr_files;
		}
		closedir(d);
	}
	return nr_files;
}

static int set_resource_files(struct vaccel_resource *res)
{
	DIR *d;
	struct dirent *dir;
	int i = 0;
	int ret;
	char filepath[MAX_FILELEN] = { 0 };

	if (!res || !res->path)
		return VACCEL_EINVAL;

	res->nr_files = get_num_files(res->path);
	if (!res->nr_files)
		return VACCEL_EINVAL;

	res->files = (struct vaccel_file **)malloc(
		res->nr_files * sizeof(struct vaccel_file *));
	if (!res->files)
		return VACCEL_ENOMEM;

	d = opendir(res->path);
	if (d) {
		memcpy(filepath, res->path, strlen(res->path));
		int baselen = strlen(res->path) + 1;
		filepath[baselen - 1] = '/';
		while ((dir = readdir(d)) != NULL) {
			if (dir->d_type == DT_REG) {
				if (strlen(dir->d_name) + baselen >=
				    MAX_FILELEN)
					return VACCEL_ENAMETOOLONG;

				memcpy(&filepath[baselen], dir->d_name,
				       strlen(dir->d_name));
				res->files[i] = (struct vaccel_file *)malloc(
					sizeof(struct vaccel_file));
				if (!res->files[i])
					return VACCEL_ENOMEM;

				ret = vaccel_file_new(res->files[i++],
						      filepath);
				if (ret)
					return ret;
			}
			memset(&filepath[baselen], 0, MAX_FILELEN - baselen);
		}
		closedir(d);
		return VACCEL_OK;
	}
	return VACCEL_EINVAL;
}

int vaccel_resource_new(struct vaccel_resource *res, char *path,
			vaccel_resource_t type)
{
	if (!initialized)
		return VACCEL_EPERM;

	if (!res || type >= VACCEL_RES_MAX || !path)
		return VACCEL_EINVAL;

	if (is_url(path)) {
		return vaccel_resource_new_from_url(res, path, type);
	} else {
		if (is_dir(path))
			return vaccel_resource_new_dir(res, path, type);
		else if (is_file(path))
			return vaccel_resource_new_file(res, path, type);
	}
	return VACCEL_EINVAL;
}

int vaccel_resource_new_file(struct vaccel_resource *res, char *path,
			     vaccel_resource_t type)
{
	if (!initialized)
		return VACCEL_EPERM;

	if (!res || type >= VACCEL_RES_MAX || !path)
		return VACCEL_EINVAL;

	res->id = id_pool_get(&id_pool);

	if (res->id <= 0)
		return VACCEL_EUSERS;

	res->remote_id = -1;
	res->type = type;
	res->file_type = FILE_LOCAL;
	res->name = NULL;
	res->path = strdup(path);
	res->nr_files = 1;
	res->deps = NULL;
	res->nr_deps = 0;

	res->files =
		(struct vaccel_file **)malloc(sizeof(struct vaccel_file *));
	if (!res->files)
		return VACCEL_ENOMEM;

	res->files[0] =
		(struct vaccel_file *)malloc(sizeof(struct vaccel_file));

	if (res->files[0] == NULL) {
		free(res->files);
		return VACCEL_ENOMEM;
	}

	int ret = vaccel_file_new(*res->files, path);
	if (ret) {
		free(res->files[0]);
		free(res->files);
		return ret;
	}
	list_init_entry(&res->entry);
	list_add_tail(&live_resources[0], &res->entry);
	atomic_init(&res->refcount, 0);
	res->rundir = NULL;

	return VACCEL_OK;
}

int vaccel_resource_new_from_buf(struct vaccel_resource *res, void *buf,
				 size_t nr_bytes, vaccel_resource_t type)
{
	int ret;

	if (!initialized)
		return VACCEL_EPERM;

	if (!res || type >= VACCEL_RES_MAX || !buf || !nr_bytes)
		return VACCEL_EINVAL;

	res->id = id_pool_get(&id_pool);

	if (res->id <= 0)
		return VACCEL_EUSERS;

	res->remote_id = -1;
	res->type = type;
	res->file_type = FILE_LOCAL;
	res->name = NULL;
	res->path = NULL;
	res->nr_files = 1;
	res->deps = NULL;
	res->nr_deps = 0;

	list_init_entry(&res->entry);
	list_add_tail(&live_resources[0], &res->entry);
	atomic_init(&res->refcount, 0);

	res->files =
		(struct vaccel_file **)malloc(sizeof(struct vaccel_file *));
	if (!res->files) {
		ret = VACCEL_ENOMEM;
		goto destroy_resource;
	}

	res->files[0] =
		(struct vaccel_file *)malloc(sizeof(struct vaccel_file));
	if (!res->files[0]) {
		free(res->files);
		ret = VACCEL_ENOMEM;
		goto destroy_resource;
	}

	ret = vaccel_file_from_buffer(res->files[0], buf, nr_bytes, NULL, NULL,
				      false, false);
	if (ret)
		goto free_mem;

	ret = resource_create_rundir(res);
	if (ret)
		goto destroy_file;

	vaccel_debug("New rundir for resource %s", res->rundir);

	ret = vaccel_file_persist(res->files[0], res->rundir, "lib.so", true);
	if (ret)
		goto destroy_file;

	return VACCEL_OK;

destroy_file:
	vaccel_file_destroy(res->files[0]);

free_mem:
	free(res->files[0]);
	free(res->files);

destroy_resource:
	vaccel_resource_destroy(res);

	return ret;
}

int vaccel_resource_new_from_url(struct vaccel_resource *res, char *path,
				 vaccel_resource_t type)
{
	if (!initialized)
		return VACCEL_EPERM;

	if (!res || type >= VACCEL_RES_MAX || !path)
		return VACCEL_EINVAL;

	res->id = id_pool_get(&id_pool);
	if (res->id <= 0)
		return VACCEL_EUSERS;

	res->remote_id = -1;
	res->type = type;
	res->file_type = FILE_REMOTE;
	res->path = strdup(path);
	res->name = NULL;
	res->nr_files = 1;
	res->deps = NULL;
	res->nr_deps = 0;

	list_init_entry(&res->entry);
	list_add_tail(&live_resources[0], &res->entry);
	atomic_init(&res->refcount, 0);
	res->rundir = NULL;

	return VACCEL_OK;
}

int vaccel_resource_new_dir(struct vaccel_resource *res, char *path,
			    vaccel_resource_t type)
{
	if (!initialized)
		return VACCEL_EPERM;

	if (!res || type >= VACCEL_RES_MAX || !path)
		return VACCEL_EINVAL;

	res->id = id_pool_get(&id_pool);
	if (res->id <= 0)
		return VACCEL_EUSERS;

	res->remote_id = -1;
	res->type = type;
	res->file_type = DIRECTORY;
	res->name = NULL;
	res->path = strdup(path);
	res->deps = NULL;
	res->nr_deps = 0;

	set_resource_files(res);

	list_init_entry(&res->entry);
	list_add_tail(&live_resources[0], &res->entry);
	atomic_init(&res->refcount, 0);
	res->rundir = NULL;

	return VACCEL_OK;
}

int vaccel_resource_new_multi(struct vaccel_resource *res, char **paths,
			      vaccel_resource_t type, size_t nr_files)
{
	size_t i;
	int ret;

	if (!initialized)
		return VACCEL_EPERM;

	if (!res || type >= VACCEL_RES_MAX || !paths || !nr_files)
		return VACCEL_EINVAL;

	res->id = id_pool_get(&id_pool);

	if (res->id <= 0)
		return VACCEL_EUSERS;

	res->files = (struct vaccel_file **)malloc(
		nr_files * sizeof(struct vaccel_file *));

	if (res->files == NULL)
		return VACCEL_ENOMEM;

	for (i = 0; i < nr_files; ++i) {
		if (!paths[i])
			goto exit;

		res->files[i] = (struct vaccel_file *)malloc(
			sizeof(struct vaccel_file));

		if (res->files[i] == NULL) {
			ret = VACCEL_ENOMEM;
			goto exit;
		}
		ret = vaccel_file_new(res->files[i], paths[i]);
		if (ret) {
			ret = VACCEL_EINVAL;
			goto exit;
		}
	}

	res->remote_id = -1;
	res->type = type;
	res->path = NULL;
	res->file_type = FILE_LIST;
	res->nr_files = nr_files;
	res->deps = NULL;
	res->nr_deps = 0;

	list_init_entry(&res->entry);
	list_add_tail(&live_resources[0], &res->entry);
	atomic_init(&res->refcount, 0);
	res->rundir = NULL;

	return VACCEL_OK;
exit:
	for (size_t j = 0; j < i; ++j) {
		vaccel_file_destroy(res->files[j]);
		free(res->files[j]);
	}
	free(res->files);

	return ret;
}

int vaccel_path_by_name(struct vaccel_resource *res, const char *name,
			char *dest)
{
	if (!initialized)
		return VACCEL_EPERM;

	if (!res || !name || !dest)
		return VACCEL_EINVAL;

	for (size_t i = 0; i < res->nr_files; i++) {
		if (strcmp(basename(res->files[i]->path), name)) {
			strcpy(dest, res->files[i]->path);
			return VACCEL_OK;
		}
	}
	return VACCEL_ENOENT;
}

char *vaccel_resource_get_path(struct vaccel_resource *res)
{
	if (!initialized || !res)
		return NULL;

	if (!res->nr_files || !res->files || !(*res->files) ||
	    !(*res->files)->path)
		return NULL;

	return strdup((*res->files)->path);
}
