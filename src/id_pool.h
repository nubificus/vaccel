/*
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __GENID_H__
#define __GENID_H__

#include <stdatomic.h>
#include <stdint.h>

#include "include/vaccel_id.h"

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

#endif /* __GENID_H__ */
