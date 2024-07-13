// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "include/vaccel_id.h"
#include <stdatomic.h>
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

/* Create and initialize a new id pool */
int id_pool_new(id_pool_t *pool, int nr_ids);

/* Destroy a pool id */
int id_pool_destroy(id_pool_t *pool);

/* Get a new id from the pool */
vaccel_id_t id_pool_get(id_pool_t *pool);

/* Release an id back to the pool */
void id_pool_release(id_pool_t *pool, vaccel_id_t id);

#ifdef __cplusplus
}
#endif
