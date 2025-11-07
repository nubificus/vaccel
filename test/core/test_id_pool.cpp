// SPDX-License-Identifier: Apache-2.0

/*
 * The code below performs unit testing to id_pool.
 *
 * 1) id_pool_init()
 * 2) id_pool_release()
 * 3) id_pool_new()
 * 4) id_pool_delete()
 * 5) id_pool_get()
 * 6) id_pool_put()
 *
 */

#include "vaccel.h"
#include <bits/pthreadtypes.h>
#include <catch2/catch_test_macros.hpp>
#include <cinttypes>
#include <cstdio>
#include <cstdlib>
#include <pthread.h>
#include <unistd.h>

enum { TEST_IDS_MAX = 100 };

// Initialize an ID pool
TEST_CASE("id_pool_init", "[core][id_pool]")
{
	id_pool_t test_pool;
	test_pool.ids = nullptr;

	SECTION("success")
	{
		REQUIRE(id_pool_init(&test_pool, TEST_IDS_MAX) == VACCEL_OK);
		REQUIRE(test_pool.ids != nullptr);
		REQUIRE(test_pool.max == TEST_IDS_MAX);
		REQUIRE(test_pool.last == 0);
		REQUIRE(id_pool_release(&test_pool) == VACCEL_OK);
	}

	SECTION("invalid arguments")
	{
		REQUIRE(id_pool_init(&test_pool, 0) == VACCEL_EINVAL);
		REQUIRE(id_pool_init(nullptr, TEST_IDS_MAX) == VACCEL_EINVAL);
	}
}

// Release an ID pool's data
TEST_CASE("id_pool_release", "[core][id_pool]")
{
	id_pool_t test_pool;
	test_pool.ids = nullptr;

	SECTION("success")
	{
		REQUIRE(id_pool_init(&test_pool, TEST_IDS_MAX) == VACCEL_OK);
		REQUIRE(id_pool_release(&test_pool) == VACCEL_OK);
	}

	SECTION("invalid arguments")
	{
		REQUIRE(id_pool_release(nullptr) == VACCEL_EINVAL);
	}
}

// Allocate and initialize an ID pool
TEST_CASE("id_pool_new", "[core][id_pool]")
{
	id_pool_t *test_pool = nullptr;

	SECTION("success")
	{
		REQUIRE(id_pool_new(&test_pool, TEST_IDS_MAX) == VACCEL_OK);
		REQUIRE(test_pool != nullptr);
		REQUIRE(test_pool->ids != nullptr);
		REQUIRE(test_pool->max == TEST_IDS_MAX);
		REQUIRE(test_pool->last == 0);
		REQUIRE(id_pool_delete(test_pool) == VACCEL_OK);
	}

	SECTION("invalid arguments")
	{
		REQUIRE(id_pool_new(&test_pool, 0) == VACCEL_EINVAL);
		REQUIRE(id_pool_new(nullptr, TEST_IDS_MAX) == VACCEL_EINVAL);
	}
}

// Release data and free an ID pool
TEST_CASE("id_pool_delete", "[core][id_pool]")
{
	id_pool_t *test_pool;

	SECTION("success")
	{
		REQUIRE(id_pool_new(&test_pool, TEST_IDS_MAX) == VACCEL_OK);
		REQUIRE(id_pool_delete(test_pool) == VACCEL_OK);
	}

	SECTION("invalid arguments")
	{
		REQUIRE(id_pool_delete(nullptr) == VACCEL_EINVAL);
	}
}

// Retrieve an ID from the pool
TEST_CASE("id_pool_get", "[core][id_pool]")
{
	id_pool_t test_pool;
	REQUIRE(id_pool_init(&test_pool, 1) == VACCEL_OK);

	SECTION("success")
	{
		vaccel_id_t const id = id_pool_get(&test_pool);
		REQUIRE(id == 1);
		REQUIRE(test_pool.last == 1);
	}

	SECTION("no more ids")
	{
		vaccel_id_t id = id_pool_get(&test_pool);
		REQUIRE(id == 1);
		REQUIRE(test_pool.last == 1);

		// No more IDs
		id = id_pool_get(&test_pool);
		REQUIRE(id == -VACCEL_EUSERS);
	}

	SECTION("invalid arguments")
	{
		REQUIRE(id_pool_get(nullptr) == -VACCEL_EINVAL);
	}

	REQUIRE(id_pool_release(&test_pool) == VACCEL_OK);
}

// Return an ID to the pool
TEST_CASE("id_pool_put", "[core][id_pool]")
{
	id_pool_t test_pool;
	REQUIRE(id_pool_init(&test_pool, 1) == VACCEL_OK);

	SECTION("success")
	{
		vaccel_id_t id = id_pool_get(&test_pool);
		REQUIRE(id == 1);
		REQUIRE(test_pool.last == 1);

		REQUIRE(id_pool_put(&test_pool, 1) == VACCEL_OK);
		REQUIRE(test_pool.last == 1);

		// Verify ID `1` is available
		id = id_pool_get(&test_pool);
		REQUIRE(id == 1);
	}

	SECTION("invalid arguments")
	{
		REQUIRE(id_pool_put(nullptr, 1) == VACCEL_EINVAL);
		REQUIRE(id_pool_put(&test_pool, 2) == VACCEL_EINVAL);
		REQUIRE(id_pool_put(&test_pool, 0) == VACCEL_EINVAL);
		REQUIRE(id_pool_put(&test_pool, 1) == VACCEL_EPERM);
	}
	REQUIRE(id_pool_release(&test_pool) == VACCEL_OK);
}

// Retrieve and return multiple IDs to the pool
TEST_CASE("id_pool_get_and_put", "[core][id_pool]")
{
	id_pool_t test_pool;
	REQUIRE(id_pool_init(&test_pool, TEST_IDS_MAX) == VACCEL_OK);

	SECTION("reverse order max")
	{
		vaccel_id_t id;

		for (vaccel_id_t i = 1; i <= TEST_IDS_MAX; i++) {
			id = id_pool_get(&test_pool);
			REQUIRE(id == i);
			REQUIRE(test_pool.last == i);
		}

		for (vaccel_id_t i = TEST_IDS_MAX; i > 0; i--)
			REQUIRE(id_pool_put(&test_pool, i) == VACCEL_OK);

		// Verify returned IDs are available
		for (vaccel_id_t i = 1; i <= TEST_IDS_MAX; i++) {
			id = id_pool_get(&test_pool);
			REQUIRE(id == i);
		}
	}

	SECTION("same order max")
	{
		vaccel_id_t id;

		for (vaccel_id_t i = 1; i <= TEST_IDS_MAX; i++) {
			id = id_pool_get(&test_pool);
			REQUIRE(id == i);
			REQUIRE(test_pool.last == id);
		}

		for (vaccel_id_t i = 1; i <= TEST_IDS_MAX; i++)
			REQUIRE(id_pool_put(&test_pool, i) == VACCEL_OK);

		// Verify returned IDs are available
		for (vaccel_id_t i = 1; i <= TEST_IDS_MAX; i++) {
			id = id_pool_get(&test_pool);
			REQUIRE(id == i);
		}
	}

	SECTION("reverse order partial")
	{
		vaccel_id_t id;
		const vaccel_id_t max_id = TEST_IDS_MAX / 2;

		for (vaccel_id_t i = 1; i <= max_id; i++) {
			id = id_pool_get(&test_pool);
			REQUIRE(id == i);
			REQUIRE(test_pool.last == i);
		}

		for (vaccel_id_t i = max_id; i > 0; i--)
			REQUIRE(id_pool_put(&test_pool, i) == VACCEL_OK);

		// Verify rest of the IDs are available
		for (vaccel_id_t i = max_id + 1; i <= TEST_IDS_MAX; i++) {
			id = id_pool_get(&test_pool);
			REQUIRE(id == i);
		}

		// Verify returned IDs are available
		for (vaccel_id_t i = 1; i <= max_id; i++) {
			id = id_pool_get(&test_pool);
			REQUIRE(id == i);
		}
	}

	SECTION("same order partial")
	{
		vaccel_id_t id;
		const vaccel_id_t max_id = TEST_IDS_MAX / 2;

		for (vaccel_id_t i = 1; i <= max_id; i++) {
			id = id_pool_get(&test_pool);
			REQUIRE(id == i);
			REQUIRE(test_pool.last == i);
		}

		for (vaccel_id_t i = 1; i <= max_id; i++)
			REQUIRE(id_pool_put(&test_pool, i) == VACCEL_OK);

		// Verify rest of the IDs are available
		for (vaccel_id_t i = max_id + 1; i <= TEST_IDS_MAX; i++) {
			id = id_pool_get(&test_pool);
			REQUIRE(id == i);
		}

		// Verify returned IDs are available
		for (vaccel_id_t i = 1; i <= max_id; i++) {
			id = id_pool_get(&test_pool);
			REQUIRE(id == i);
		}
	}

	SECTION("reverse out of order max")
	{
		vaccel_id_t id;

		for (vaccel_id_t i = 1; i <= TEST_IDS_MAX; i++) {
			id = id_pool_get(&test_pool);
			REQUIRE(id == i);
			REQUIRE(test_pool.last == i);
		}

		for (vaccel_id_t i = TEST_IDS_MAX; i > 0; i--)
			if (i % 2 == 0)
				REQUIRE(id_pool_put(&test_pool, i) ==
					VACCEL_OK);

		// Verify returned IDs are available
		for (vaccel_id_t i = 1; i <= TEST_IDS_MAX; i++) {
			if (i % 2 == 0) {
				id = id_pool_get(&test_pool);
				REQUIRE(id == i);
			}
		}
	}

	SECTION("same out of order max")
	{
		vaccel_id_t id;

		for (vaccel_id_t i = 1; i <= TEST_IDS_MAX; i++) {
			id = id_pool_get(&test_pool);
			REQUIRE(id == i);
			REQUIRE(test_pool.last == i);
		}

		for (vaccel_id_t i = 1; i <= TEST_IDS_MAX; i++)
			if (i % 2 == 0)
				REQUIRE(id_pool_put(&test_pool, i) ==
					VACCEL_OK);

		// Verify returned IDs are available
		for (vaccel_id_t i = 1; i <= TEST_IDS_MAX; i++) {
			if (i % 2 == 0) {
				id = id_pool_get(&test_pool);
				REQUIRE(id == i);
			}
		}
	}

	SECTION("reverse out of order partial")
	{
		vaccel_id_t id;
		const vaccel_id_t max_id = TEST_IDS_MAX / 2;

		for (vaccel_id_t i = 1; i <= max_id; i++) {
			id = id_pool_get(&test_pool);
			REQUIRE(id == i);
			REQUIRE(test_pool.last == i);
		}

		for (vaccel_id_t i = max_id; i > 0; i--)
			if (i % 2 == 0)
				REQUIRE(id_pool_put(&test_pool, i) ==
					VACCEL_OK);

		// Verify rest of the IDs are available
		for (vaccel_id_t i = max_id + 1; i <= TEST_IDS_MAX; i++) {
			id = id_pool_get(&test_pool);
			REQUIRE(id == i);
		}

		// Verify returned IDs are available
		for (vaccel_id_t i = 1; i <= max_id; i++) {
			if (i % 2 == 0) {
				id = id_pool_get(&test_pool);
				REQUIRE(id == i);
			}
		}
	}

	SECTION("same out of order partial")
	{
		vaccel_id_t id;
		const vaccel_id_t max_id = TEST_IDS_MAX / 2;

		for (vaccel_id_t i = 1; i <= max_id; i++) {
			id = id_pool_get(&test_pool);
			REQUIRE(id == i);
			REQUIRE(test_pool.last == i);
		}

		for (vaccel_id_t i = 1; i <= max_id; i++)
			if (i % 2 == 0)
				REQUIRE(id_pool_put(&test_pool, i) ==
					VACCEL_OK);

		// Verify rest of the IDs are available
		for (vaccel_id_t i = max_id + 1; i <= TEST_IDS_MAX; i++) {
			id = id_pool_get(&test_pool);
			REQUIRE(id == i);
		}

		// Verify returned IDs are available
		for (vaccel_id_t i = 1; i <= max_id; i++) {
			if (i % 2 == 0) {
				id = id_pool_get(&test_pool);
				REQUIRE(id == i);
			}
		}
	}

	REQUIRE(id_pool_release(&test_pool) == VACCEL_OK);
}

enum { TEST_THREADS_NUM = 50, TEST_THREAD_IDS_NUM = 20 };

struct thread_data {
	int id;
	id_pool_t *pool;
};

static auto get_and_put_ids(void *arg) -> void *
{
	auto *data = (struct thread_data *)arg;

	for (int i = 0; i < TEST_THREAD_IDS_NUM; i++) {
		vaccel_id_t const id = id_pool_get(data->pool);
		REQUIRE(id > 0);
		printf("Thread %d retrieved ID: %" PRId64 "\n", data->id, id);

		// Add random delay to simulate work
		usleep(rand() % 1000);

		REQUIRE(id_pool_put(data->pool, id) == VACCEL_OK);
		printf("Thread %d returned ID: %" PRId64 "\n", data->id, id);
	}
	return nullptr;
}

// Concurrently retrieve and return multiple IDs to the pool
TEST_CASE("id_pool_get_and_put_concurrent", "[core][id_pool]")
{
	id_pool_t test_pool;
	REQUIRE(id_pool_init(&test_pool, TEST_IDS_MAX) == VACCEL_OK);

	pthread_t threads[TEST_THREADS_NUM];
	struct thread_data thread_data[TEST_THREADS_NUM];

	for (int i = 0; i < TEST_THREADS_NUM; i++) {
		thread_data[i].pool = &test_pool;
		thread_data[i].id = i;
		pthread_create(&threads[i], nullptr, get_and_put_ids,
			       &thread_data[i]);
	}

	for (unsigned long const thread : threads) {
		pthread_join(thread, nullptr);
	}

	REQUIRE(id_pool_release(&test_pool) == VACCEL_OK);
}
