// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <stddef.h>

#ifdef __cplusplus
#include <atomic>
typedef std::atomic<unsigned int> atomic_uint;
#else
#include <stdatomic.h>
#endif

#include "error.h"
#include "vaccel_id.h"
#include "list.h"
#include "vaccel_file.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	VACCEL_FILE_LIB = 0,
	VACCEL_FILE_DATA,
	VACCEL_RES_MAX
} vaccel_resource_t;

typedef enum { FILE_LOCAL, FILE_REMOTE, DIRECTORY, FILE_LIST } vaccel_file_t;

struct vaccel_resource {
	/* resource id */
	vaccel_id_t id;

	/* remote id of the remote resource */
	vaccel_id_t remote_id;

	/* type of the resource */
	vaccel_resource_t type;

	/* type of the given path */
	vaccel_file_t file_type;

	/* An entry to add this resource in a list */
	list_entry_t entry;

	/* Reference counter representing the number of sessions
	 * to which this resource is registered to. */
	atomic_uint refcount;

	/* name of the resource */
	char *name;

	/* path of the resource */
	char *path;

	/* rundir for this resource if it needs it. It can be empty (NULL) */
	char *rundir;

	/* resource representation of the files. can be an array */
	struct vaccel_file **files;

	/* number of entities represented by the resource */
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
int vaccel_resource_new(struct vaccel_resource *res, char *path,
			vaccel_resource_t type);
int vaccel_resource_new_from_buf(struct vaccel_resource *res, void *buf,
				 size_t nr_bytes, vaccel_resource_t type);
int vaccel_resource_new_multi(struct vaccel_resource *res, char **paths,
			      vaccel_resource_t type, size_t nr_files);
int vaccel_resource_destroy(struct vaccel_resource *res);

int vaccel_path_by_name(struct vaccel_resource *res, const char *name,
			char *dest);
char *vaccel_resource_get_path(struct vaccel_resource *res);

#ifdef __cplusplus
}
#endif
