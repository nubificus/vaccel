// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "include/id.h" // IWYU pragma: export
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
#include <atomic>
#ifndef atomic_bool
typedef std::atomic<bool> atomic_bool;
#endif
#ifndef atomic_int64_t
typedef std::atomic<int64_t> atomic_int64_t;
#endif
#else
#include <stdatomic.h>
#ifndef atomic_int64_t
typedef _Atomic int64_t atomic_int64_t;
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct id_pool {
	/* Pool of ids */
	atomic_bool *ids;

	/* Maximum ids available in this pool */
	vaccel_id_t max;

	/* Last used id */
	atomic_int64_t last;
} id_pool_t;

/* Initialize id pool */
int id_pool_init(id_pool_t *pool, vaccel_id_t nr_ids);

/* Release id pool data */
int id_pool_release(id_pool_t *pool);

/* Allocate and initialize id pool */
int id_pool_new(id_pool_t **pool, vaccel_id_t nr_ids);

/* Release id pool data and free id pool created with id_pool_new() */
int id_pool_delete(id_pool_t *pool);

/* Retrieve a new id from the pool */
vaccel_id_t id_pool_get(id_pool_t *pool);

/* Return an id to the pool */
int id_pool_put(id_pool_t *pool, vaccel_id_t id);

#ifdef __cplusplus
}
#endif
