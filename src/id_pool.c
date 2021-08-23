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

#include <stdlib.h>

#include "id_pool.h"
#include "error.h"

int id_pool_new(id_pool_t *pool, int nr_ids)
{
	if (!nr_ids)
		return VACCEL_EINVAL;

	pool->ids = calloc(nr_ids, sizeof(vaccel_id_t));
	if (!pool->ids)
		return VACCEL_ENOMEM;

	pool->max = nr_ids;
	atomic_init(&pool->next, 0);

	return VACCEL_OK;

}

int id_pool_destroy(id_pool_t *pool)
{
	if (!pool)
		return VACCEL_EINVAL;

	if (pool->ids)
		free(pool->ids);

	return VACCEL_OK;
}

vaccel_id_t id_pool_get(id_pool_t *pool)
{
	if (!pool)
		return 0;

	int ptr = atomic_fetch_add(&pool->next, 1);
	if (ptr >= pool->max)
		return 0;

	if (!pool->ids[ptr])
		pool->ids[ptr] = ptr + 1;

	return pool->ids[ptr];
}

void id_pool_release(id_pool_t *pool, vaccel_id_t id)
{
	if (!pool)
		return;

	if (!id || id > pool->max)
		return;

	int curr = pool->next;
	int ptr = curr - 1;

	/* Atomically decrease the pointer value */
	while (!atomic_compare_exchange_strong(&pool->next, &curr, ptr))
		ptr = curr - 1;

	pool->ids[ptr] = id;
}
