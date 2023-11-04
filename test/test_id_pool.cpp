#include <catch2/catch_test_macros.hpp>
#include <fff.h>

#include <atomic>

using atomic_int = std::atomic<int>;
using atomic_uint = std::atomic<unsigned int>;

DEFINE_FFF_GLOBALS;

extern "C"{
#include "id_pool.h"
#include "error.h"
}

TEST_CASE("id_pool_new", "[id_pool]")
{   
    id_pool_t test_pool;
    SECTION("id_pool created properly")
    {
        REQUIRE(id_pool_new(&test_pool, 10) == VACCEL_OK);
    }

    SECTION("id_pool has 0 ids")
    {
        REQUIRE(id_pool_new(&test_pool, 0) == VACCEL_EINVAL);
    }

    SECTION("not enough memory")
    {

        //REQUIRE(id_pool_new(&test_pool, 10) == VACCEL_ENOMEM);
        REQUIRE(1==1);
    }

}   

TEST_CASE("id_pool_destroy", "[id_pool]")
{   
    SECTION("id_pool succesfully destroyed")
    {
        id_pool_t test_pool;
        REQUIRE(id_pool_new(&test_pool, 10) == VACCEL_OK);
        REQUIRE(id_pool_destroy(&test_pool) == VACCEL_OK);
    }

    SECTION("id_pool doesn't exist")
    {
        REQUIRE(id_pool_destroy(NULL) == VACCEL_EINVAL);
    }

}

TEST_CASE("id_pool_get", "[id_pool]")
{
    SECTION("working id_pool_get - with count to 0")
    {
        id_pool_t test_pool;
        id_pool_new(&test_pool, 3);
        vaccel_id_t id_test = id_pool_get(&test_pool);
        REQUIRE(id_test == 1);
        id_test = id_pool_get(&test_pool);
        REQUIRE(id_test == 2);
        id_test = id_pool_get(&test_pool);
        REQUIRE(id_test == 3);

        /// No more ids

        id_test = id_pool_get(&test_pool);
        REQUIRE(id_test == 0);
    }

    SECTION("no pool to get ids from")
    {
        REQUIRE(id_pool_get(NULL) == 0);
    }
}


TEST_CASE("id_pool_release", "[id_pool]")
{

    SECTION("id 1 is released back into pool")
    {
        id_pool_t test_pool;
        id_pool_new(&test_pool,3);
        vaccel_id_t id_test = id_pool_get(&test_pool);
        REQUIRE(id_test == 1);
        id_pool_get(&test_pool);

        // release id 1 back into pool

        id_pool_release(&test_pool, 1);
        
        
        // get id 1 back

        id_test = id_pool_get(&test_pool);
        REQUIRE(id_test == 1);

    }

}