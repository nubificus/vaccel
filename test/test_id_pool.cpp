#include "gtest/gtest.h"

extern "C" {
#include "id_pool.h"
}

TEST(id_pool_create_no_elements, not_ok) {
	ASSERT_EQ(id_pool_new(0), nullptr);
}

TEST(id_pool_create_elements, ok) {
	ASSERT_NE(id_pool_new(10), nullptr);
}

TEST(id_pool_get_from_invalid_pool, ok) {
	ASSERT_EQ(get_new_id(nullptr), 0);
}

TEST(id_pool_get_unavailable, not_ok) {
	id_pool_t *pool = id_pool_new(1);

	ASSERT_NE(pool, nullptr);

	uint32_t id = get_new_id(pool);
	ASSERT_EQ(id, 1);
	ASSERT_EQ(get_new_id(pool), 0);
}

TEST(id_pool_mix_get_put, ok) {
	id_pool_t *pool = id_pool_new(10);

	ASSERT_NE(pool, nullptr);

	ASSERT_EQ(get_new_id(pool), 1);
	ASSERT_EQ(get_new_id(pool), 2);
	ASSERT_EQ(get_new_id(pool), 3);
	release_id(pool, 2);
	ASSERT_EQ(get_new_id(pool), 2);
	ASSERT_EQ(get_new_id(pool), 4);
	release_id(pool, 1);
	ASSERT_EQ(get_new_id(pool), 1);
	ASSERT_EQ(get_new_id(pool), 5);
}
