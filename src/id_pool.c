#include <stdlib.h>

#include "id_pool.h"

id_pool_t *id_pool_new(size_t max_ids)
{
	if (!max_ids)
		return NULL;

	id_pool_t *ret = malloc(sizeof(*ret));
	if (!ret)
		return NULL;

	/* Initialize all the ids to 0. We will give them actual values
	 * the first time a new, i.e. not a re-used id is being created */
	ret->ids = calloc(max_ids, sizeof(*ret->ids));
	if (!ret)
		goto cleanup;

	ret->max = max_ids;
	ret->next_free = 0;
	if (pthread_spin_init(&ret->lock, PTHREAD_PROCESS_PRIVATE))
		goto cleanup_ids;

	return ret;

cleanup_ids:
	free(ret->ids);
cleanup:
	free(ret);
	return NULL;
}

void id_pool_destory(id_pool_t *pool)
{
	if (!pool)
		return;

	if (pool->ids)
		free(pool->ids);

	free(pool);
}

uint32_t get_new_id(id_pool_t *pool)
{
	uint32_t id;

	if (!pool)
		return 0;

	pthread_spin_lock(&pool->lock);
	size_t ptr = pool->next_free;

	if (ptr == pool->max) {
		id = 0;
	} else {
		/* If this is the first time we get this id (we have not
		 * reused it), assign to it now a value (at the moment this
		 * value is just the sequential number) */
		if (!pool->ids[ptr])
			pool->ids[ptr] = ptr + 1;

		id = pool->ids[ptr];
		pool->next_free++;
	}
	pthread_spin_unlock(&pool->lock);

	return id;
}

void release_id(id_pool_t *pool, uint32_t id)
{
	pthread_spin_lock(&pool->lock);
	pool->ids[--pool->next_free] = id;
	pthread_spin_unlock(&pool->lock);
}
