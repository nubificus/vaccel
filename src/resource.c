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
#include "utils.h"

#include <assert.h>
#include <dirent.h>
#include <libgen.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

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
			vaccel_resource_release(res);
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
	if (!res || !res->id) {
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

	int ret = mkdir(rundir, S_IRUSR | S_IWUSR | S_IXUSR);
	if (ret) {
		vaccel_error("Could not create rundir %s: %s", rundir,
			     strerror(errno));
		return ret;
	}

	res->rundir = strndup(rundir, MAX_RESOURCE_RUNDIR);
	if (!res->rundir)
		return VACCEL_ENOMEM;

	vaccel_debug("New rundir for resource %s", res->rundir);

	return VACCEL_OK;
}

void resource_destroy_rundir(struct vaccel_resource *res)
{
	if (cleanup_rundir(res->rundir))
		vaccel_warn("Could not cleanup rundir %s: %s", res->rundir,
			    strerror(errno));

	free(res->rundir);
}

static bool path_is_dir(const char *path)
{
	if (!path)
		return false;

	struct stat path_stat;
	if (stat(path, &path_stat) < 0)
		return false;

	return S_ISDIR(path_stat.st_mode);
}

static bool path_is_file(const char *path)
{
	if (!path)
		return false;

	struct stat path_stat;
	if (stat(path, &path_stat) < 0)
		return false;

	return S_ISREG(path_stat.st_mode);
}

static bool path_is_url(const char *path)
{
	const char *prefix = "http://";
	if (strncmp(path, prefix, strlen(prefix)) == 0)
		return true;

	return false;
}

static vaccel_path_t path_get_type(const char *path)
{
	if (path_is_url(path))
		return VACCEL_PATH_REMOTE;

	if (path_is_dir(path))
		return VACCEL_PATH_DIR;

	if (path_is_file(path))
		return VACCEL_PATH_LOCAL;

	return VACCEL_PATH_MAX;
}

static int get_num_files(const char *path)
{
	// FIXME: print errors
	if (!path)
		return VACCEL_EINVAL;

	DIR *d = opendir(path);
	if (!d)
		return VACCEL_EINVAL;

	struct dirent *dir;
	int nr_files = 0;
	while ((dir = readdir(d)) != NULL) {
		if (dir->d_type == DT_REG)
			++nr_files;
	}

	closedir(d);

	return nr_files;
}

static void delete_files(struct vaccel_file **files, size_t nr_files)
{
	if (!files)
		return;

	for (size_t i = 0; i < nr_files; i++) {
		if (!files[i])
			continue;

		int ret = vaccel_file_delete(files[i]);
		if (ret)
			vaccel_warn("Could not delete file %zu", i);
	}
}

static int resource_add_files_from_dir(struct vaccel_resource *res,
				       bool with_data)
{
	int ret;

	// FIXME: print errors
	if (!res || !res->paths || !res->paths[0] || res->nr_paths != 1)
		return VACCEL_EINVAL;

	size_t nr_dirfiles = get_num_files(res->paths[0]);
	if (!nr_dirfiles)
		return VACCEL_EINVAL;

	DIR *d = opendir(res->paths[0]);
	if (!d)
		return VACCEL_EINVAL;

	struct vaccel_file **files = (struct vaccel_file **)malloc(
		nr_dirfiles * sizeof(struct vaccel_file *));
	if (!files) {
		ret = VACCEL_ENOMEM;
		goto close_dir;
	}

	for (size_t i = 0; i < nr_dirfiles; i++)
		files[i] = NULL;

	size_t pathlen = strlen(res->paths[0]) + 1;
	if (pathlen >= MAX_FILELEN) {
		ret = VACCEL_ENAMETOOLONG;
		goto free_files;
	}

	char filepath[MAX_FILELEN] = { 0 };
	memcpy(filepath, res->paths[0], pathlen);
	size_t baselen = strlen(filepath) + 1;
	filepath[baselen - 1] = '/';
	size_t nr_files = 0;
	struct dirent *dir;
	while ((dir = readdir(d)) != NULL) {
		if (nr_files > nr_dirfiles) {
			vaccel_error(
				"Computed and actual file numbers differ (%zu vs %zu)",
				nr_dirfiles, nr_files);
			ret = VACCEL_EINVAL;
			goto free;
		}
		if (dir->d_type == DT_REG) {
			size_t filenamelen = strlen(dir->d_name) + 1;
			if (filenamelen + baselen >= MAX_FILELEN) {
				ret = VACCEL_ENAMETOOLONG;
				goto free;
			}

			memcpy(&filepath[baselen], dir->d_name, filenamelen);

			ret = vaccel_file_new(&files[nr_files], filepath);
			if (ret)
				goto free;

			++nr_files;

			if (with_data) {
				ret = vaccel_file_read(files[nr_files - 1]);
				if (ret)
					goto free;
			}
		}
		memset(&filepath[baselen], 0, MAX_FILELEN - baselen);
	}

	closedir(d);

	assert(nr_dirfiles == nr_files);
	res->files = files;
	res->nr_files = nr_files;

	return VACCEL_OK;

free:
	delete_files(files, nr_files);
free_files:
	free(files);
close_dir:
	closedir(d);

	return ret;
}

static int resource_add_files_from_local(struct vaccel_resource *res,
					 bool with_data)
{
	int ret;

	if (!res || !res->paths || !res->nr_paths)
		return VACCEL_EINVAL;

	struct vaccel_file **files = (struct vaccel_file **)malloc(
		res->nr_paths * sizeof(struct vaccel_file *));
	if (!files)
		return VACCEL_ENOMEM;

	for (size_t i = 0; i < res->nr_paths; i++)
		files[i] = NULL;

	size_t nr_files = 0;
	for (size_t i = 0; i < res->nr_paths; i++) {
		if (!res->paths[i]) {
			ret = VACCEL_EINVAL;
			goto free;
		}

		ret = vaccel_file_new(&files[i], res->paths[i]);
		if (ret)
			goto free;

		++nr_files;

		if (with_data) {
			ret = vaccel_file_read(files[i]);
			if (ret)
				goto free;
		}
	}

	assert(res->nr_paths == nr_files);
	res->files = files;
	res->nr_files = nr_files;

	return VACCEL_OK;

free:
	delete_files(files, nr_files);
	free(files);

	return ret;
}

static int resource_init_common_with_paths(struct vaccel_resource *res,
					   char **paths, size_t nr_paths,
					   vaccel_resource_t type)
{
	int ret;

	if (!res || !paths || !nr_paths || type >= VACCEL_RESOURCE_MAX)
		return VACCEL_EINVAL;

	res->id = id_pool_get(&id_pool);
	if (res->id <= 0)
		return VACCEL_EUSERS;

	if (nr_paths > 1) {
		// TODO: make this bitwise to hold specific path type
		res->path_type = VACCEL_PATH_LIST;
	} else {
		res->path_type = path_get_type(paths[0]);
		if (res->path_type == VACCEL_PATH_MAX) {
			ret = VACCEL_EINVAL;
			goto release_id;
		}
	}

	res->paths = paths;
	res->nr_paths = nr_paths;

	res->remote_id = -1;
	res->type = type;
	res->files = NULL;
	res->nr_files = 0;
	res->deps = NULL;
	res->nr_deps = 0;
	res->rundir = NULL;
	atomic_init(&res->refcount, 0);

	list_init_entry(&res->entry);
	list_add_tail(&live_resources[res->type], &res->entry);

	vaccel_debug("Initialized resource %lld", res->id);

	return VACCEL_OK;

release_id:
	id_pool_release(&id_pool, res->id);

	return ret;
}

static int resource_init_common_with_files(struct vaccel_resource *res,
					   struct vaccel_file **files,
					   size_t nr_files,
					   vaccel_resource_t type)
{
	if (!res || !files || !nr_files || type >= VACCEL_RESOURCE_MAX ||
	    !res->rundir || !res->id)
		return VACCEL_EINVAL;

	res->files = files;
	res->nr_files = nr_files;

	res->remote_id = -1;
	res->type = type;
	res->path_type = VACCEL_PATH_LOCAL;
	res->paths = NULL;
	res->nr_paths = 0;
	res->deps = NULL;
	res->nr_deps = 0;
	atomic_init(&res->refcount, 0);

	list_init_entry(&res->entry);
	list_add_tail(&live_resources[res->type], &res->entry);

	vaccel_debug("Initialized resource %lld", res->id);

	return VACCEL_OK;
}

int vaccel_resource_init(struct vaccel_resource *res, const char *path,
			 vaccel_resource_t type)
{
	int ret;

	if (!initialized)
		return VACCEL_EPERM;

	if (!res || !path || type >= VACCEL_RESOURCE_MAX)
		return VACCEL_EINVAL;

	char **r_paths = (char **)malloc(sizeof(char *));
	if (!r_paths)
		return VACCEL_ENOMEM;

	r_paths[0] = strdup(path);
	if (!r_paths[0]) {
		ret = VACCEL_ENOMEM;
		goto free_paths;
	}

	ret = resource_init_common_with_paths(res, r_paths, 1, type);
	if (ret)
		goto free;

	return VACCEL_OK;

free:
	free(r_paths[0]);
free_paths:
	free(r_paths);

	return ret;
}

int vaccel_resource_init_multi(struct vaccel_resource *res, const char **paths,
			       size_t nr_paths, vaccel_resource_t type)
{
	int ret;

	if (!initialized)
		return VACCEL_EPERM;

	if (!res || !paths || !nr_paths || type >= VACCEL_RESOURCE_MAX)
		return VACCEL_EINVAL;

	char **r_paths = (char **)malloc(nr_paths * sizeof(char *));
	if (!r_paths)
		return VACCEL_ENOMEM;

	for (size_t i = 0; i < nr_paths; i++)
		r_paths[i] = NULL;

	for (size_t i = 0; i < nr_paths; i++) {
		if (!paths[i]) {
			ret = VACCEL_EINVAL;
			goto free;
		}

		r_paths[i] = strdup(paths[i]);
		if (!r_paths[i]) {
			ret = VACCEL_ENOMEM;
			goto free;
		}
	}

	ret = resource_init_common_with_paths(res, r_paths, nr_paths, type);
	if (ret)
		goto free;

	return VACCEL_OK;

free:
	for (size_t i = 0; i < nr_paths; i++) {
		if (!r_paths[i])
			continue;

		free(r_paths[i]);
	}
	free(r_paths);

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

	res->id = id_pool_get(&id_pool);
	if (res->id <= 0)
		return VACCEL_EUSERS;

	struct vaccel_file **r_files =
		(struct vaccel_file **)malloc(sizeof(struct vaccel_file *));
	if (!r_files) {
		ret = VACCEL_ENOMEM;
		goto release_id;
	}

	ret = resource_create_rundir(res);
	if (ret)
		goto free_files;

	if (filename != NULL) {
		ret = vaccel_file_from_buf(&r_files[0], buf, nr_bytes, filename,
					   res->rundir, false);
	} else {
		ret = vaccel_file_from_buf(&r_files[0], buf, nr_bytes, "file",
					   res->rundir, true);
	}
	if (ret)
		goto cleanup_rundir;

	ret = resource_init_common_with_files(res, r_files, 1, type);
	if (ret)
		goto delete_file;

	return VACCEL_OK;

delete_file:
	delete_files(r_files, 1);
cleanup_rundir:
	resource_destroy_rundir(res);
free_files:
	free(r_files);
release_id:
	id_pool_release(&id_pool, res->id);

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

	res->id = id_pool_get(&id_pool);
	if (res->id <= 0)
		return VACCEL_EUSERS;

	struct vaccel_file **r_files = (struct vaccel_file **)malloc(
		nr_files * sizeof(struct vaccel_file *));
	if (!r_files) {
		ret = VACCEL_ENOMEM;
		goto release_id;
	}

	for (size_t i = 0; i < nr_files; i++)
		r_files[i] = NULL;

	ret = resource_create_rundir(res);
	if (ret)
		goto free_files;

	size_t nr_r_files = 0;
	for (size_t i = 0; i < nr_files; i++) {
		if (!files[i]) {
			ret = VACCEL_EINVAL;
			goto free;
		}

		ret = vaccel_file_from_buf(&r_files[i], files[i]->data,
					   files[i]->size, files[i]->name,
					   res->rundir, false);
		if (ret)
			goto free;

		++nr_r_files;
	}
	assert(nr_r_files == nr_files);

	ret = resource_init_common_with_files(res, r_files, nr_r_files, type);
	if (ret)
		goto free;

	return VACCEL_OK;

free:
	delete_files(r_files, nr_r_files);
	resource_destroy_rundir(res);
free_files:
	free(r_files);
release_id:
	id_pool_release(&id_pool, res->id);

	return ret;
}

int vaccel_resource_release(struct vaccel_resource *res)
{
	if (!initialized)
		return VACCEL_EPERM;

	if (!res)
		return VACCEL_EINVAL;

	/* Check if this resource is currently registered to a session.
	 * We do not destroy currently-used resources */
	if (atomic_load(&res->refcount)) {
		vaccel_error("Cannot destroy used resource %lld", res->id);
		return VACCEL_EBUSY;
	}

	if (res->files) {
		delete_files(res->files, res->nr_files);
		free(res->files);
		res->files = NULL;
	}
	res->nr_files = 0;

	id_pool_release(&id_pool, res->id);
	res->id = -1;

	list_unlink_entry(&res->entry);

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

	if (res->deps || res->nr_deps)
		vaccel_warn("Resource %lld has deps that will not be destroyed",
			    res->id);

	return VACCEL_OK;
}

int vaccel_resource_new(struct vaccel_resource **res, const char *path,
			vaccel_resource_t type)
{
	struct vaccel_resource *r = (struct vaccel_resource *)malloc(
		sizeof(struct vaccel_resource));
	if (!r)
		return VACCEL_ENOMEM;

	int ret = vaccel_resource_init(r, path, type);
	if (!ret) {
		free(r);
		return ret;
	}

	*res = r;

	return VACCEL_OK;
}

int vaccel_resource_multi_new(struct vaccel_resource **res, const char **paths,
			      size_t nr_paths, vaccel_resource_t type)
{
	struct vaccel_resource *r = (struct vaccel_resource *)malloc(
		sizeof(struct vaccel_resource));
	if (!r)
		return VACCEL_ENOMEM;

	int ret = vaccel_resource_init_multi(r, paths, nr_paths, type);
	if (!ret) {
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
	struct vaccel_resource *r = (struct vaccel_resource *)malloc(
		sizeof(struct vaccel_resource));
	if (!r)
		return VACCEL_ENOMEM;

	int ret = vaccel_resource_init_from_files(r, files, nr_files, type);
	if (!ret) {
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

	if (!res->nr_files) {
		switch (res->path_type) {
		case VACCEL_PATH_LOCAL:
		case VACCEL_PATH_LIST:
			ret = resource_add_files_from_local(res,
							    sess->is_virtio);
			break;
		case VACCEL_PATH_DIR:
			ret = resource_add_files_from_dir(res, sess->is_virtio);
			break;
		case VACCEL_PATH_REMOTE:
			// TODO:
			// ret = resource_add_files_from_remote(res, !sess->is_virtio);
			vaccel_error("Resource from URL is not implemented");
			ret = VACCEL_ENOTSUP;
			break;
		case VACCEL_PATH_MAX:
			vaccel_error("Invalid path type");
			ret = VACCEL_EINVAL;
			break;
		}
		if (ret) {
			vaccel_error("Failed to add resource files: %d", ret);
			return ret;
		}
	}

	if (sess->is_virtio) {
		struct vaccel_plugin *virtio = get_virtio_plugin();
		if (virtio) {
			ret = virtio->info->resource_register(res, sess);
			if (res->remote_id <= 0 || ret) {
				vaccel_error(
					"Could not register remote resource");
				return ret;
			}
		} else {
			vaccel_error(
				"Could not register resource to virtio session, no VirtIO Plugin loaded yet");
			return VACCEL_ENOTSUP;
		}
	}

	ret = session_register_resource(sess, res);
	if (ret)
		return ret;

	if (sess->is_virtio) {
		vaccel_debug(
			"Registered resource %lld (remote id: %lld) with session %" PRIu32
			" (remote id: %" PRIu32 ")",
			res->id, res->remote_id, sess->session_id,
			sess->remote_id);
	} else {
		vaccel_debug("Registered resource %lld with session %" PRIu32,
			     res->id, sess->session_id);
	}

	return VACCEL_OK;
}

int vaccel_resource_unregister(struct vaccel_resource *res,
			       struct vaccel_session *sess)
{
	int ret = session_unregister_resource(sess, res);
	if (ret) {
		vaccel_error(
			"Could not unregister resource %lld from session %" PRIu32,
			res->id, sess->session_id);
		return ret;
	}

	if (sess->is_virtio) {
		struct vaccel_plugin *virtio = get_virtio_plugin();
		if (virtio) {
			ret = virtio->info->resource_unregister(res, sess);
			if (ret) {
				vaccel_error(
					"Could not unregister remote resource %" PRIu32,
					res->remote_id);
				return ret;
			}
		} else {
			vaccel_error(
				"Could not unregister resource for virtio session, no VirtIO Plugin loaded yet");
			return VACCEL_ENOTSUP;
		}

		vaccel_debug(
			"Unregistered resource %lld (remote id: %lld) from session %" PRIu32
			" (remote id: %" PRIu32 ")",
			res->id, res->remote_id, sess->session_id,
			sess->remote_id);
	} else {
		vaccel_debug("Unregistered resource %lld from session %" PRIu32,
			     res->id, sess->session_id);
	}

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
