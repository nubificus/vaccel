// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "include/id.h"
#ifdef __cplusplus
#include <atomic>
#ifndef atomic_int
typedef std::atomic<int> atomic_int;
#endif
#else
#include <stdatomic.h>
#endif
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct id_pool {
	/* Pool of ids */
	vaccel_id_t *ids;

	/* Maximum ids available in this pool */
	int max;

	/* Next available id */
	atomic_int next;
} id_pool_t;

/* Initialize id pool */
int id_pool_init(id_pool_t *pool, int nr_ids);

/* Release id pool data */
int id_pool_release(id_pool_t *pool);

/* Allocate and initialize id pool */
int id_pool_new(id_pool_t **pool, int nr_ids);

/* Release id pool data and free id pool created with id_pool_new() */
int id_pool_delete(id_pool_t *pool);

/* Retrieve a new id from the pool */
vaccel_id_t id_pool_get(id_pool_t *pool);

/* Return an id to the pool */
void id_pool_put(id_pool_t *pool, vaccel_id_t id);

#ifdef __cplusplus
}
#endif
