// SPDX-License-Identifier: Apache-2.0

#pragma once

#ifdef __cplusplus
#include <atomic>
typedef std::atomic<unsigned int> atomic_uint;
#else
#include <stdatomic.h>
#endif
#include "error.h"
#include "file.h"
#include "list.h"
#include "session.h"
#include "vaccel_id.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	VACCEL_RESOURCE_LIB = 0,
	VACCEL_RESOURCE_DATA,
	VACCEL_RESOURCE_MODEL,
	VACCEL_RESOURCE_MAX
} vaccel_resource_t;

typedef enum {
	VACCEL_PATH_LOCAL = 0,
	VACCEL_PATH_REMOTE,
	VACCEL_PATH_DIR,
	VACCEL_PATH_LIST,
	VACCEL_PATH_MAX
} vaccel_path_t;

struct vaccel_resource {
	/* an entry to add this resource in a list */
	list_entry_t entry;

	/* resource id */
	vaccel_id_t id;

	/* remote id of the remote resource */
	vaccel_id_t remote_id;

	/* type of the resource */
	vaccel_resource_t type;

	/* type of the given path */
	vaccel_path_t path_type;

	/* reference counter representing the number of sessions
	 * to which this resource is registered to */
	atomic_uint refcount;

	/* path of the resource. can be an array */
	char **paths;

	/* number of path entities represented by the resource */
	size_t nr_paths;

	/* rundir for this resource if it needs it. can be empty (NULL) */
	char *rundir;

	/* resource representation of the file. can be an array */
	struct vaccel_file **files;

	/* number of file entities represented by the resource */
	size_t nr_files;

	struct vaccel_resource **deps;

	size_t nr_deps;
};

int vaccel_resource_get_deps(struct vaccel_resource ***deps, size_t *nr_deps,
			     struct vaccel_resource *res);

int vaccel_resource_deps_to_ids(vaccel_id_t *ids, struct vaccel_resource **deps,
				size_t nr_deps);

int vaccel_resource_deps_from_ids(struct vaccel_resource **deps,
				  vaccel_id_t *ids, size_t nr_ids);

int vaccel_resource_set_deps_from_ids(struct vaccel_resource *res,
				      vaccel_id_t *ids, size_t nr_ids);

int vaccel_resource_init(struct vaccel_resource *res, const char *path,
			 vaccel_resource_t type);

int vaccel_resource_init_multi(struct vaccel_resource *res, const char **paths,
			       size_t nr_paths, vaccel_resource_t type);

int vaccel_resource_init_from_buf(struct vaccel_resource *res, const void *buf,
				  size_t nr_bytes, vaccel_resource_t type,
				  const char *filename);

int vaccel_resource_init_from_files(struct vaccel_resource *res,
				    const struct vaccel_file **files,
				    size_t nr_files, vaccel_resource_t type);

int vaccel_resource_release(struct vaccel_resource *res);

int vaccel_resource_new(struct vaccel_resource **res, const char *path,
			vaccel_resource_t type);

int vaccel_resource_from_buf(struct vaccel_resource **res, const void *buf,
			     size_t nr_bytes, vaccel_resource_t type);

int vaccel_resource_from_files(struct vaccel_resource **res,
			       const struct vaccel_file **files,
			       size_t nr_files, vaccel_resource_t type);

int vaccel_resource_delete(struct vaccel_resource *res);

int vaccel_resource_register(struct vaccel_resource *res,
			     struct vaccel_session *sess);

int vaccel_resource_unregister(struct vaccel_resource *res,
			       struct vaccel_session *sess);

int vaccel_resource_get_path_by_name(struct vaccel_resource *res,
				     const char *name, char *dest);

char *vaccel_resource_get_path_by_index(struct vaccel_resource *res,
					size_t idx);

#define vaccel_resource_get_path(res) vaccel_resource_get_path_by_index(res, 0)

int vaccel_resource_get_by_id(struct vaccel_resource **resource,
			      vaccel_id_t id);

#ifdef __cplusplus
}
#endif
