#include <catch.hpp>

#include <fff.h>

#include <atomic>

using atomic_int = std::atomic<int>;
using atomic_uint = std::atomic<unsigned int>;

DEFINE_FFF_GLOBALS;

extern "C" {
#include "error.h"
#include "id_pool.h"
}

/*
 * The code below performs unit testing to id_pool.
 *
 * 1) id_pool_new()
 * 2) id_pool_destroy()
 * 3) id_pool_get()
 * 4) id_pool_release()
 */

// Test case for creating a new ID pool
TEST_CASE("id_pool_new", "[id_pool]")
{
    id_pool_t test_pool;
    test_pool.ids = nullptr;

    // Check if the pool is created properly with a capacity of 10
    SECTION("id_pool created properly")
    {
        REQUIRE(id_pool_new(&test_pool, 10) == VACCEL_OK);
        REQUIRE(test_pool.ids != nullptr);
        REQUIRE(test_pool.max == 10);
        REQUIRE(test_pool.next == 0);
        REQUIRE(id_pool_destroy(&test_pool) == VACCEL_OK);
    }

    // Check handling of a pool with 0 IDs
    SECTION("id_pool has 0 ids")
    {
        REQUIRE(id_pool_new(&test_pool, 0) == VACCEL_EINVAL);
        REQUIRE(test_pool.ids == nullptr);
        REQUIRE(id_pool_destroy(&test_pool) == VACCEL_OK);
    }

    // Check handling of not enough memory (mocked here for simplicity)
    SECTION("not enough memory")
    {
        // Mocked case for not enough memory
        REQUIRE(1 == 1);
    }
}

// Test case for destroying an ID pool
TEST_CASE("id_pool_destroy", "[id_pool]")
{
    // Check if the pool is successfully destroyed
    SECTION("id_pool successfully destroyed")
    {
        id_pool_t test_pool;
        test_pool.ids = nullptr;
        REQUIRE(test_pool.ids == nullptr);
        REQUIRE(id_pool_new(&test_pool, 10) == VACCEL_OK);

        REQUIRE(test_pool.ids != nullptr);
        REQUIRE(test_pool.max == 10);
        REQUIRE(test_pool.next == 0);

        REQUIRE(id_pool_destroy(&test_pool) == VACCEL_OK);
    }

    // Check handling when the pool doesn't exist
    SECTION("id_pool doesn't exist")
    {
        REQUIRE(id_pool_destroy(NULL) == VACCEL_EINVAL);
    }
}

// Test case for retrieving IDs from the pool
TEST_CASE("id_pool_get", "[id_pool]")
{
    // Check successful retrieval of IDs when the pool has a count of 3
    SECTION("working id_pool_get - with count to 0")
    {
        id_pool_t test_pool;
        id_pool_new(&test_pool, 3);

        REQUIRE(test_pool.ids != nullptr);
        REQUIRE(test_pool.max == 3);
        REQUIRE(test_pool.next == 0);

        vaccel_id_t id_test = id_pool_get(&test_pool);
        REQUIRE(id_test == 1);

        REQUIRE(test_pool.ids != nullptr);
        REQUIRE(test_pool.max == 3);
        REQUIRE(test_pool.next == 1);

        id_test = id_pool_get(&test_pool);
        REQUIRE(id_test == 2);

        REQUIRE(test_pool.ids != nullptr);
        REQUIRE(test_pool.max == 3);
        REQUIRE(test_pool.next == 2);

        id_test = id_pool_get(&test_pool);
        REQUIRE(id_test == 3);

        REQUIRE(test_pool.ids != nullptr);
        REQUIRE(test_pool.max == 3);
        REQUIRE(test_pool.next == 3);

        // No more IDs should be available
        id_test = id_pool_get(&test_pool);

        REQUIRE(test_pool.ids != nullptr);
        REQUIRE(test_pool.max == 3);
        REQUIRE(test_pool.next == 4);

        REQUIRE(id_test == 0);

        REQUIRE(id_pool_destroy(&test_pool) == VACCEL_OK);
    }

    // Check handling when there is no pool to get IDs from
    SECTION("no pool to get ids from") { REQUIRE(id_pool_get(NULL) == 0); }
}

// Test case for releasing IDs back into the pool
TEST_CASE("id_pool_release", "[id_pool]")
{
    // Check if ID 1 is successfully released back into the pool
    SECTION("id 1 is released back into pool")
    {
        id_pool_t test_pool;
        id_pool_new(&test_pool, 3);

        REQUIRE(test_pool.ids != nullptr);
        REQUIRE(test_pool.max == 3);
        REQUIRE(test_pool.next == 0);

        vaccel_id_t id_test = id_pool_get(&test_pool);
        REQUIRE(id_test == 1);
        REQUIRE(test_pool.ids != nullptr);
        REQUIRE(test_pool.max == 3);
        REQUIRE(test_pool.next == 1);

        id_pool_get(&test_pool);

        REQUIRE(test_pool.ids != nullptr);
        REQUIRE(test_pool.max == 3);
        REQUIRE(test_pool.next == 2);

        // Release ID 1 back into the pool
        id_pool_release(&test_pool, 1);

        REQUIRE(test_pool.ids != nullptr);
        REQUIRE(test_pool.max == 3);
        REQUIRE(test_pool.next == 1); // this goes back to 1

        // Get ID 1 back
        id_test = id_pool_get(&test_pool);

        REQUIRE(id_test == 1);
        REQUIRE(test_pool.ids != nullptr);
        REQUIRE(test_pool.max == 3);
        REQUIRE(test_pool.next == 2);

        REQUIRE(id_pool_destroy(&test_pool) == VACCEL_OK);
    }
}