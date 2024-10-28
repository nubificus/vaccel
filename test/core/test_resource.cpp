// SPDX-License-Identifier: Apache-2.0

/*
 *
 * The code below performs unit testing to resources.
 *
 * 1) resource_bootstrap()
 * 2) resource_new()
 * 3) resource_destroy()
 * 4) resource_create_rundir()
 * 5) vaccel_resource_get_by_id()
 *
 */

#include <catch.hpp>
#include <utils.hpp>

#include <vaccel.h>

// Mock cleanup function for resources
auto cleanup_resource_mock([[maybe_unused]] void *data) -> int
{
	return 0;
}

// Test case for resource destruction
TEST_CASE("resource_destroy", "[resources]")
{
	int ret;
	struct vaccel_resource res;
	vaccel_resource_t test_type = VACCEL_RESOURCE_LIB;
	char *test_path = abs_path(BUILD_ROOT, "examples/libmytestlib.so");

	// Test handling of null resource
	SECTION("Null resource")
	{
		ret = vaccel_resource_init(nullptr, nullptr, test_type);
		REQUIRE(ret == VACCEL_EINVAL);

		ret = vaccel_resource_release(nullptr);
		REQUIRE(ret == VACCEL_EINVAL);
	}

	// Test creation and destruction of a valid resource
	SECTION("Valid resource")
	{
		ret = vaccel_resource_init(&res, test_path, test_type);
		REQUIRE(ret == VACCEL_OK);

		REQUIRE(res.id == 1);
		REQUIRE(res.type == VACCEL_RESOURCE_LIB);
		REQUIRE_FALSE(list_empty(&res.entry));
		REQUIRE(res.refcount == 0);
		REQUIRE(res.rundir == NULL);

		ret = vaccel_resource_release(&res);
		REQUIRE(ret == VACCEL_OK);

		REQUIRE(res.id == -1);
		REQUIRE(list_empty(&res.entry));
		REQUIRE(res.refcount == 0);
		REQUIRE(res.rundir == NULL);
	}
	free(test_path);
}

// Test case for resource creation and rundir creation
TEST_CASE("resource_create", "[resources]")
{
	int ret;
	struct vaccel_resource res;
	vaccel_resource_t test_type = VACCEL_RESOURCE_LIB;
	char *test_path = abs_path(BUILD_ROOT, "examples/libmytestlib.so");

	// Create a resource
	ret = vaccel_resource_init(&res, test_path, test_type);
	REQUIRE(ret == VACCEL_OK);

	REQUIRE(res.id == 1);
	REQUIRE(res.type == VACCEL_RESOURCE_LIB);
	REQUIRE_FALSE(list_empty(&res.entry));
	REQUIRE(res.refcount == 0);
	REQUIRE(res.rundir == NULL);

	// Test rundir creation
	ret = resource_create_rundir(&res);
	REQUIRE(ret == VACCEL_OK);

	REQUIRE(res.id == 1);
	REQUIRE(res.type == VACCEL_RESOURCE_LIB);
	REQUIRE_FALSE(list_empty(&res.entry));
	REQUIRE(res.refcount == 0);
	REQUIRE_FALSE(res.rundir == NULL);

	// Cleanup the resource
	ret = vaccel_resource_release(&res);
	REQUIRE(ret == VACCEL_OK);

	REQUIRE(res.id == -1);
	REQUIRE(list_empty(&res.entry));
	REQUIRE(res.refcount == 0);
	REQUIRE(res.rundir == NULL);

	free(test_path);
}

// Test case for finding a resource by ID (failure case)
TEST_CASE("resource_find_by_id_fail", "[resources]")
{
	struct vaccel_resource *test_res = nullptr;
	vaccel_id_t test_id = 0;

	// Attempt to find a resource by ID which fails (ID of 0 doesn't exist -
	// starts at 1)
	int ret = vaccel_resource_get_by_id(&test_res, test_id);
	REQUIRE(ret == VACCEL_EINVAL);
}

// Test case for finding a resource by ID (success case)
TEST_CASE("resource_find_by_id", "[resources]")
{
	int ret;
	struct vaccel_resource test_res;
	vaccel_resource_t test_type = VACCEL_RESOURCE_LIB;
	char *test_path = abs_path(BUILD_ROOT, "examples/libmytestlib.so");

	// Create a test resource
	ret = vaccel_resource_init(&test_res, test_path, test_type);
	REQUIRE(ret == VACCEL_OK);

	REQUIRE(test_res.id == 1);
	REQUIRE(test_res.type == VACCEL_RESOURCE_LIB);
	REQUIRE_FALSE(list_empty(&test_res.entry));
	REQUIRE(test_res.refcount == 0);
	REQUIRE(test_res.rundir == NULL);

	// Attempt to find the resource by ID and ensure success
	struct vaccel_resource *result_resource = nullptr;
	vaccel_id_t id_to_find = 1;

	ret = vaccel_resource_get_by_id(&result_resource, id_to_find);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(result_resource != nullptr);

	REQUIRE(result_resource->id == 1);
	REQUIRE(result_resource->type == VACCEL_RESOURCE_LIB);
	REQUIRE_FALSE(list_empty(&result_resource->entry));
	REQUIRE(result_resource->refcount == 0);
	REQUIRE(result_resource->rundir == NULL);

	// Cleanup the test resource
	ret = vaccel_resource_release(&test_res);
	REQUIRE(ret == VACCEL_OK);

	REQUIRE(test_res.id == -1);
	REQUIRE(list_empty(&test_res.entry));
	REQUIRE(test_res.refcount == 0);
	REQUIRE(test_res.rundir == NULL);

	free(test_path);
}

TEST_CASE("resource_not_bootstrapped", "[resources]")
{
	int ret;
	struct vaccel_resource test_res;
	vaccel_resource_t test_type = VACCEL_RESOURCE_LIB;
	char *test_path = abs_path(BUILD_ROOT, "examples/libmytestlib.so");
	struct vaccel_resource *result_resource = nullptr;
	vaccel_id_t id_to_find = 1;

	// cleanup here so resources are not bootstrapped
	ret = resources_cleanup();
	REQUIRE(ret == VACCEL_OK);

	ret = vaccel_resource_init(&test_res, test_path, test_type);
	REQUIRE(ret == VACCEL_EPERM);

	ret = vaccel_resource_get_by_id(&result_resource, id_to_find);
	REQUIRE(ret == VACCEL_EPERM);

	ret = vaccel_resource_release(&test_res);
	REQUIRE(ret == VACCEL_EPERM);

	ret = resource_create_rundir(nullptr);
	REQUIRE(ret == VACCEL_EINVAL);

	// bootstrap again so the rest of the tests run correctly
	ret = resources_bootstrap();
	REQUIRE(ret == VACCEL_OK);

	free(test_path);
}
