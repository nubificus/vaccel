// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "file.h"
#include "id.h"
#include "list.h"
#include "session.h"
#include "utils/path.h"
#include <stddef.h>

#ifdef __cplusplus
#include <atomic>
#ifndef atomic_uint
typedef std::atomic<unsigned int> atomic_uint;
#endif
#else
#include <stdatomic.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	VACCEL_RESOURCE_LIB = 0,
	VACCEL_RESOURCE_DATA,
	VACCEL_RESOURCE_MODEL,
	VACCEL_RESOURCE_MAX
} vaccel_resource_t;

struct vaccel_resource {
	/* an entry to add this resource in a list */
	vaccel_list_entry_t entry;

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
};

/* Get resource by index from live resources */
int vaccel_resource_get_by_id(struct vaccel_resource **resource,
			      vaccel_id_t id);

/* Get refcount atomically */
long int vaccel_resource_refcount(const struct vaccel_resource *res);

/* Initialize resource */
int vaccel_resource_init(struct vaccel_resource *res, const char *path,
			 vaccel_resource_t type);

/* Initialize resource with multiple file paths */
int vaccel_resource_init_multi(struct vaccel_resource *res, const char **paths,
			       size_t nr_paths, vaccel_resource_t type);

/* Initialize resource from in-memory data */
int vaccel_resource_init_from_buf(struct vaccel_resource *res, const void *buf,
				  size_t nr_bytes, vaccel_resource_t type,
				  const char *filename);

/* Initialize resource from existing vaccel files */
int vaccel_resource_init_from_files(struct vaccel_resource *res,
				    const struct vaccel_file **files,
				    size_t nr_files, vaccel_resource_t type);

/* Release resource data */
int vaccel_resource_release(struct vaccel_resource *res);

/* Allocate and initialize resource */
int vaccel_resource_new(struct vaccel_resource **res, const char *path,
			vaccel_resource_t type);

/* Allocate and initialize resource with multiple file paths */
int vaccel_resource_multi_new(struct vaccel_resource **res, const char **paths,
			      size_t nr_paths, vaccel_resource_t type);

/* Allocate and initialize resource from in-memory data */
int vaccel_resource_from_buf(struct vaccel_resource **res, const void *buf,
			     size_t nr_bytes, vaccel_resource_t type,
			     const char *filename);

/* Allocate and initialize resource from existing vaccel files */
int vaccel_resource_from_files(struct vaccel_resource **res,
			       const struct vaccel_file **files,
			       size_t nr_files, vaccel_resource_t type);

/* Release resource data and free resource created with
 * vaccel_resource_new*() or vaccel_resource_from_*() */
int vaccel_resource_delete(struct vaccel_resource *res);

/* Register resource with session */
int vaccel_resource_register(struct vaccel_resource *res,
			     struct vaccel_session *sess);

/* Unregister resource from session */
int vaccel_resource_unregister(struct vaccel_resource *res,
			       struct vaccel_session *sess);

/* Get directory of a resource created from a directory.
 * If an alloc_path is provided the resulting path string will be allocated and
 * returned there. If not, the path will be copied to out_path.
 * IMPORTANT: If alloc_path == NULL an out_path/out_path_size big enough to
 * hold the resource directory path must be provided */
int vaccel_resource_directory(struct vaccel_resource *res, char *out_path,
			      size_t out_path_size, char **alloc_path);

#ifdef __cplusplus
}
#endif
