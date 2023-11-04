#include <catch2/catch_test_macros.hpp>

#include <atomic>

using atomic_int = std::atomic<int>;
using atomic_uint = std::atomic<unsigned int>;

extern "C" {
#include "resources.h"
#include "id_pool.h"
#include "error.h"
#include "list.h"
#include "log.h"
#include "utils.h"
#include "vaccel.h"
#include "plugin.h"
}

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <sys/stat.h>
int cleanup_resource_mock([[maybe_unused]] void *data){
    return 0;
}

TEST_CASE("destroy_OK", "[Resources]") {
    int ret;
    struct vaccel_resource res;
    vaccel_resource_t test_type = VACCEL_RES_TF_MODEL;
    void* test_data = nullptr;
    int (*cleanup_res_test)(void*) = cleanup_resource_mock;

    ret = resources_bootstrap();
    REQUIRE(ret == VACCEL_OK);

    SECTION("Null resource")
    {
        ret = resource_new(NULL, test_type, test_data, cleanup_res_test);
        REQUIRE(ret == VACCEL_EINVAL);

    }

    SECTION("Valid resource")
    {
        ret = resource_new(&res, test_type, test_data, cleanup_res_test);
        REQUIRE(ret == VACCEL_OK);

        ret = resource_destroy(&res);
        REQUIRE(ret == VACCEL_OK);

    }

}

TEST_CASE("Resource Create Rundir", "[Resources]") {

    int ret;
    struct vaccel_resource res;
    vaccel_resource_t test_type = VACCEL_RES_TF_MODEL;
    void* test_data = nullptr;
    int (*cleanup_res_test)(void*) = cleanup_resource_mock;

    ret = resources_bootstrap();
    REQUIRE(ret == VACCEL_OK);

    ret = resource_new(&res, test_type, test_data, cleanup_res_test);
    REQUIRE(ret == VACCEL_OK);

    ret = resource_create_rundir(&res);
    REQUIRE(ret == VACCEL_OK);

    ret = resource_destroy(&res);
    REQUIRE(ret == VACCEL_OK);

}

TEST_CASE("find_resource_by_id_fail", "[Resources]") {
    struct vaccel_resource* test_res = nullptr;
    vaccel_id_t test_id = 0;
    int ret = resource_get_by_id(&test_res, test_id);
    REQUIRE(ret == VACCEL_EINVAL);
}

TEST_CASE("find_resource_by_id", "[Resources]") {
    int result;
    struct vaccel_resource test_res;
    vaccel_resource_t test_type = VACCEL_RES_TF_MODEL;
    void* test_data = nullptr;
    int (*cleanup_res_test)(void*) = cleanup_resource_mock;

    result = resources_bootstrap();
    REQUIRE(result == VACCEL_OK);
    result = resource_new(&test_res, test_type, test_data, cleanup_res_test);
    REQUIRE(result == VACCEL_OK);

    struct vaccel_resource* result_resource = nullptr;
    vaccel_id_t id_to_find = 1;

    int ret = resource_get_by_id(&result_resource, id_to_find);
    REQUIRE(ret == VACCEL_OK);
    REQUIRE(result_resource != nullptr);

    result = resource_destroy(&test_res);
    REQUIRE(result == VACCEL_OK);
}
