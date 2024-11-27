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
 */

#include <catch.hpp>
#include <utils.hpp>
#include <vaccel.h>

// Initialize an ID pool
TEST_CASE("id_pool_init", "[core][id_pool]")
{
	id_pool_t test_pool;
	test_pool.ids = nullptr;

	SECTION("success")
	{
		REQUIRE(id_pool_init(&test_pool, 10) == VACCEL_OK);
		REQUIRE(test_pool.ids != nullptr);
		REQUIRE(test_pool.max == 10);
		REQUIRE(test_pool.next == 0);
		REQUIRE(id_pool_release(&test_pool) == VACCEL_OK);
	}

	SECTION("invalid arguments")
	{
		REQUIRE(id_pool_init(&test_pool, 0) == VACCEL_EINVAL);
		REQUIRE(id_pool_init(nullptr, 10) == VACCEL_EINVAL);
	}
}

// Release an ID pool's data
TEST_CASE("id_pool_release", "[core][id_pool]")
{
	id_pool_t test_pool;
	test_pool.ids = nullptr;

	SECTION("success")
	{
		REQUIRE(id_pool_init(&test_pool, 10) == VACCEL_OK);
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
		REQUIRE(id_pool_new(&test_pool, 10) == VACCEL_OK);
		REQUIRE(test_pool != nullptr);
		REQUIRE(test_pool->ids != nullptr);
		REQUIRE(test_pool->max == 10);
		REQUIRE(test_pool->next == 0);
		REQUIRE(id_pool_delete(test_pool) == VACCEL_OK);
	}

	SECTION("invalid arguments")
	{
		REQUIRE(id_pool_new(&test_pool, 0) == VACCEL_EINVAL);
		REQUIRE(id_pool_new(nullptr, 10) == VACCEL_EINVAL);
	}
}

// Release data and free an ID pool
TEST_CASE("id_pool_delete", "[core][id_pool]")
{
	id_pool_t *test_pool;

	SECTION("success")
	{
		REQUIRE(id_pool_new(&test_pool, 10) == VACCEL_OK);
		REQUIRE(id_pool_delete(test_pool) == VACCEL_OK);
	}

	SECTION("invalid arguments")
	{
		REQUIRE(id_pool_delete(nullptr) == VACCEL_EINVAL);
	}
}

// Retrieve IDs from the pool
TEST_CASE("id_pool_get", "[core][id_pool]")
{
	id_pool_t test_pool;
	REQUIRE(id_pool_init(&test_pool, 3) == VACCEL_OK);

	SECTION("success with 3 ids")
	{
		vaccel_id_t id_test = id_pool_get(&test_pool);
		REQUIRE(id_test == 1);
		REQUIRE(test_pool.next == 1);

		id_test = id_pool_get(&test_pool);
		REQUIRE(id_test == 2);
		REQUIRE(test_pool.next == 2);

		id_test = id_pool_get(&test_pool);
		REQUIRE(id_test == 3);
		REQUIRE(test_pool.next == 3);

		// No more IDs
		id_test = id_pool_get(&test_pool);
		REQUIRE(id_test == 0);
		REQUIRE(test_pool.next == 4);
	}

	SECTION("invalid arguments")
	{
		REQUIRE(id_pool_get(nullptr) == 0);
	}

	REQUIRE(id_pool_release(&test_pool) == VACCEL_OK);
}

// Release IDs back into the pool
TEST_CASE("id_pool_put", "[core][id_pool]")
{
	id_pool_t test_pool;
	REQUIRE(id_pool_init(&test_pool, 3) == VACCEL_OK);

	SECTION("success")
	{
		vaccel_id_t id_test = id_pool_get(&test_pool);
		REQUIRE(id_test == 1);

		id_pool_get(&test_pool);
		REQUIRE(test_pool.next == 2);

		id_pool_put(&test_pool, 1);
		REQUIRE(test_pool.next == 1); // this goes back to 1

		// Get ID 1 back
		id_test = id_pool_get(&test_pool);
		REQUIRE(id_test == 1);
		REQUIRE(test_pool.next == 2);
	}

	REQUIRE(id_pool_release(&test_pool) == VACCEL_OK);
}
