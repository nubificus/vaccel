// SPDX-License-Identifier: Apache-2.0

#include <stdbool.h>
#include <stdlib.h>

#include "id_pool.h"
#include "error.h"
#include "log.h"

int id_pool_init(id_pool_t *pool, vaccel_id_t nr_ids)
{
	if (!pool || nr_ids <= 0)
		return VACCEL_EINVAL;

	pool->ids = calloc(nr_ids, sizeof(atomic_bool));
	if (!pool->ids)
		return VACCEL_ENOMEM;

	for (vaccel_id_t i = 0; i < nr_ids; i++)
		atomic_init(&pool->ids[i], true);

	pool->max = nr_ids;
	atomic_init(&pool->last, 0);

	return VACCEL_OK;
}

int id_pool_release(id_pool_t *pool)
{
	if (!pool)
		return VACCEL_EINVAL;

	if (pool->ids)
		free(pool->ids);

	return VACCEL_OK;
}

int id_pool_new(id_pool_t **pool, vaccel_id_t nr_ids)
{
	if (!pool || nr_ids <= 0)
		return VACCEL_EINVAL;

	id_pool_t *p = (id_pool_t *)malloc(sizeof(id_pool_t));
	if (!p)
		return VACCEL_ENOMEM;

	int ret = id_pool_init(p, nr_ids);
	if (ret) {
		free(p);
		return ret;
	}

	*pool = p;

	return VACCEL_OK;
}

int id_pool_delete(id_pool_t *pool)
{
	int ret = id_pool_release(pool);
	if (ret)
		return ret;

	free(pool);

	return VACCEL_OK;
}

vaccel_id_t id_pool_get(id_pool_t *pool)
{
	if (!pool)
		return -VACCEL_EINVAL;

	vaccel_id_t cur = atomic_load(&pool->last);

	/* Try finding an available ID sequentially first */
	for (vaccel_id_t id = cur; id < pool->max; id++) {
		bool available = true;
		vaccel_id_t next = id + 1;
		if (atomic_compare_exchange_strong(&pool->ids[id], &available,
						   false)) {
			atomic_store(&pool->last, next);
			return next;
		}
	}

	vaccel_warn("Reached max ID (%" PRId64 ")", pool->max);
	vaccel_warn("Trying to reclaim lower IDs");

	/* If max ID has been reached, reset `last` */
	if (cur == pool->max)
		atomic_store(&pool->last, 0);

	/* If no sequential ID is available, search for any freed IDs */
	for (vaccel_id_t id = 0; id < cur; id++) {
		bool available = true;
		vaccel_id_t next = id + 1;
		if (atomic_compare_exchange_strong(&pool->ids[id], &available,
						   false))
			return next;
	}

	return -VACCEL_EUSERS;
}

int id_pool_put(id_pool_t *pool, vaccel_id_t id)
{
	if (!pool || !id || id > pool->max || id <= 0)
		return VACCEL_EINVAL;

	vaccel_id_t idx = id - 1;

	vaccel_id_t available = atomic_load(&pool->ids[idx]);
	if (available)
		return VACCEL_EPERM;

	atomic_store(&pool->ids[idx], true);

	return VACCEL_OK;
}
