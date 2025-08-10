// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "blob.h"
#include "id.h"
#include "list.h"
#include "utils/enum.h"
#include "utils/path.h"
#include <stddef.h>
#include <sys/types.h>

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

/* Define vaccel_resource_type_t, vaccel_resource_type_to_str() and
 * vaccel_resource_type_to_base_str() */
#define _ENUM_PREFIX VACCEL_RESOURCE
#define VACCEL_RESOURCE_TYPE_ENUM_LIST(VACCEL_ENUM_ITEM) \
	VACCEL_ENUM_ITEM(LIB, 0, _ENUM_PREFIX)           \
	VACCEL_ENUM_ITEM(DATA, _ENUM_PREFIX)             \
	VACCEL_ENUM_ITEM(MODEL, _ENUM_PREFIX)

VACCEL_ENUM_DEF_WITH_STR_FUNCS(vaccel_resource_type, _ENUM_PREFIX,
			       VACCEL_RESOURCE_TYPE_ENUM_LIST)
#undef _ENUM_PREFIX

struct vaccel_resource {
	/* resource id */
	vaccel_id_t id;

	/* remote id of the remote resource */
	vaccel_id_t remote_id;

	/* type of the resource */
	vaccel_resource_type_t type;

	/* type of the given path */
	vaccel_path_type_t path_type;

	/* path of the resource. can be an array */
	char **paths;

	/* number of path entities represented by the resource */
	size_t nr_paths;

	/* fs run directory. can be empty (NULL) */
	char *rundir;

	/* resource representation of the blob. can be an array */
	struct vaccel_blob **blobs;

	/* number of blob entities represented by the resource */
	size_t nr_blobs;

	/* entry for global resources list */
	struct vaccel_list_entry entry;

	/* list of sessions the resource is registered with */
	struct vaccel_list_entry sessions;

	/* lock for session list */
	pthread_mutex_t sessions_lock;

	/* reference counter representing the number of sessions
	 * the resource is registered with */
	atomic_uint refcount;

	/* plugin private data */
	void *plugin_priv;
};

/* Get resource by index from created resources */
int vaccel_resource_get_by_id(struct vaccel_resource **res, vaccel_id_t id);

/* Get the first created resource with the given type */
int vaccel_resource_get_by_type(struct vaccel_resource **res,
				vaccel_resource_type_t type);

/* Get a list of of all created resources with the given type */
int vaccel_resource_get_all_by_type(struct vaccel_resource ***res,
				    size_t *nr_found,
				    vaccel_resource_type_t type);

/* Get refcount atomically */
long int vaccel_resource_refcount(const struct vaccel_resource *res);

/* Initialize resource */
int vaccel_resource_init(struct vaccel_resource *res, const char *path,
			 vaccel_resource_type_t type);

/* Initialize resource with multiple file paths */
int vaccel_resource_init_multi(struct vaccel_resource *res, const char **paths,
			       size_t nr_paths, vaccel_resource_type_t type);

/* Initialize resource from in-memory data */
int vaccel_resource_init_from_buf(struct vaccel_resource *res, const void *buf,
				  size_t nr_bytes, vaccel_resource_type_t type,
				  const char *filename, bool mem_only);

/* Initialize resource from existing vaccel blobs */
int vaccel_resource_init_from_blobs(struct vaccel_resource *res,
				    const struct vaccel_blob **blobs,
				    size_t nr_blobs,
				    vaccel_resource_type_t type);

/* Release resource data */
int vaccel_resource_release(struct vaccel_resource *res);

/* Allocate and initialize resource */
int vaccel_resource_new(struct vaccel_resource **res, const char *path,
			vaccel_resource_type_t type);

/* Allocate and initialize resource with multiple file paths */
int vaccel_resource_multi_new(struct vaccel_resource **res, const char **paths,
			      size_t nr_paths, vaccel_resource_type_t type);

/* Allocate and initialize resource from in-memory data */
int vaccel_resource_from_buf(struct vaccel_resource **res, const void *buf,
			     size_t nr_bytes, vaccel_resource_type_t type,
			     const char *filename, bool mem_only);

/* Allocate and initialize resource from existing vaccel blobs */
int vaccel_resource_from_blobs(struct vaccel_resource **res,
			       const struct vaccel_blob **blobs,
			       size_t nr_blobs, vaccel_resource_type_t type);

/* Release resource data and free resource created with
 * vaccel_resource_new*() or vaccel_resource_from_*() */
int vaccel_resource_delete(struct vaccel_resource *res);

struct vaccel_session;

/* Synchronize the content of the resource based on
 * possible remote changes */
int vaccel_resource_sync(struct vaccel_resource *res,
			 struct vaccel_session *sess);

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
