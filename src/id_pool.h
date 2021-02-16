#ifndef __VACCEL_ID_POOL_H__
#define __VACCEL_ID_POOL_H__

#include <stdint.h>
#include <stddef.h>
#include <pthread.h>

typedef struct {
	/* Pool of ids */
	uint32_t *ids;

	/* Maximum id available in this pool */
	uint32_t max;

	/* Pointer to next available id */
	size_t next_free;

	/* Lock to allow for thread-safe access to ids */
	pthread_spinlock_t lock;
} id_pool_t;

id_pool_t *id_pool_new(size_t max_ids);
void id_pool_destroy(id_pool_t *pool);
uint32_t get_new_id(id_pool_t *pool);
void release_id(id_pool_t *pool, uint32_t id);

#endif /* __VACCEL_ID_POOL_H__ */
