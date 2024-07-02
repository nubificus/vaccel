// SPDX-License-Identifier: Apache-2.0

/*
 *
 * The code below performs unit testing to resources.
 *
 * 1) resource_bootstrap()
 * 2) resource_new()
 * 3) resource_destroy()
 * 4) resource_create_rundir()
 * 5) resource_get_by_id()
 *
 */

#include <catch.hpp>
#include <utils.hpp>

extern "C" {
#include <vaccel.h>
}

// Mock cleanup function for resources
auto cleanup_resource_mock([[maybe_unused]] void *data) -> int
{
	return 0;
}

// Test case for resource destruction
TEST_CASE("destroy_OK", "[Resources]")
{
	int ret;
	struct vaccel_resource res;
	vaccel_resource_t test_type = VACCEL_RES_SINGLE_MODEL;
	void *test_data = nullptr;
	int (*cleanup_res_test)(void *) = cleanup_resource_mock;

	// Ensure that the resource system is initialized
	ret = resources_bootstrap();
	REQUIRE(ret == VACCEL_OK);

	// Test handling of null resource
	SECTION("Null resource")
	{
		ret = resource_new(nullptr, test_type, test_data,
				   cleanup_res_test);
		REQUIRE(ret == VACCEL_EINVAL);

		ret = resource_destroy(nullptr);
		REQUIRE(ret == VACCEL_EINVAL);
	}

	// Test creation and destruction of a valid resource
	SECTION("Valid resource")
	{
		ret = resource_new(&res, test_type, test_data,
				   cleanup_res_test);
		REQUIRE(ret == VACCEL_OK);

		REQUIRE(res.id == 1);
		REQUIRE(res.type == VACCEL_RES_SINGLE_MODEL);
		REQUIRE(res.data == nullptr);
		REQUIRE_FALSE(list_empty(&res.entry));
		REQUIRE(res.refcount == 0);
		REQUIRE(res.rundir == NULL);

		ret = resource_destroy(&res);
		REQUIRE(ret == VACCEL_OK);

		REQUIRE(res.id == 1);
		REQUIRE(res.type == VACCEL_RES_SINGLE_MODEL);
		REQUIRE(res.data == nullptr);
		REQUIRE(list_empty(&res.entry));
		REQUIRE(res.refcount == 0);
		REQUIRE(res.rundir == NULL);
	}

	ret = resources_cleanup();
	REQUIRE(ret == VACCEL_OK);
}

// Test case for resource creation and rundir creation
TEST_CASE("Resource Create Rundir", "[Resources]")
{
	int ret;
	struct vaccel_resource res;
	vaccel_resource_t test_type = VACCEL_RES_SINGLE_MODEL;
	void *test_data = nullptr;
	int (*cleanup_res_test)(void *) = cleanup_resource_mock;

	// Ensure that the resource system is initialized
	ret = resources_bootstrap();
	REQUIRE(ret == VACCEL_OK);

	// Create a resource
	ret = resource_new(&res, test_type, test_data, cleanup_res_test);
	REQUIRE(ret == VACCEL_OK);

	REQUIRE(res.id == 1);
	REQUIRE(res.type == VACCEL_RES_SINGLE_MODEL);
	REQUIRE(res.data == nullptr);
	REQUIRE_FALSE(list_empty(&res.entry));
	REQUIRE(res.refcount == 0);
	REQUIRE(res.rundir == NULL);

	// Test rundir creation
	ret = resource_create_rundir(&res);
	REQUIRE(ret == VACCEL_OK);

	REQUIRE(res.id == 1);
	REQUIRE(res.type == VACCEL_RES_SINGLE_MODEL);
	REQUIRE(res.data == nullptr);
	REQUIRE_FALSE(list_empty(&res.entry));
	REQUIRE(res.refcount == 0);
	REQUIRE_FALSE(res.rundir == NULL);

	// Cleanup the resource
	ret = resource_destroy(&res);
	REQUIRE(ret == VACCEL_OK);

	REQUIRE(res.id == 1);
	REQUIRE(res.type == VACCEL_RES_SINGLE_MODEL);
	REQUIRE(res.data == nullptr);
	REQUIRE(list_empty(&res.entry));
	REQUIRE(res.refcount == 0);
	REQUIRE_FALSE(res.rundir == NULL);

	ret = resources_cleanup();
	REQUIRE(ret == VACCEL_OK);
}

// Test case for finding a resource by ID (failure case)
TEST_CASE("find_resource_by_id_fail", "[Resources]")
{
	struct vaccel_resource *test_res = nullptr;
	vaccel_id_t test_id = 0;

	// Ensure that the resource system is initialized
	int ret = resources_bootstrap();
	REQUIRE(ret == VACCEL_OK);

	// Attempt to find a resource by ID which fails (ID of 0 doesn't exist -
	// starts at 1)
	ret = resource_get_by_id(&test_res, test_id);
	REQUIRE(ret == VACCEL_EINVAL);

	ret = resources_cleanup();
	REQUIRE(ret == VACCEL_OK);
}

// Test case for finding a resource by ID (success case)
TEST_CASE("find_resource_by_id", "[Resources]")
{
	int ret;
	struct vaccel_resource test_res;
	vaccel_resource_t test_type = VACCEL_RES_SINGLE_MODEL;
	void *test_data = nullptr;
	int (*cleanup_res_test)(void *) = cleanup_resource_mock;

	// Ensure that the resource system is initialized
	ret = resources_bootstrap();
	REQUIRE(ret == VACCEL_OK);

	// Create a test resource
	ret = resource_new(&test_res, test_type, test_data, cleanup_res_test);
	REQUIRE(ret == VACCEL_OK);

	REQUIRE(test_res.id == 1);
	REQUIRE(test_res.type == VACCEL_RES_SINGLE_MODEL);
	REQUIRE(test_res.data == nullptr);
	REQUIRE_FALSE(list_empty(&test_res.entry));
	REQUIRE(test_res.refcount == 0);
	REQUIRE(test_res.rundir == NULL);

	// Attempt to find the resource by ID and ensure success
	struct vaccel_resource *result_resource = nullptr;
	vaccel_id_t id_to_find = 1;

	ret = resource_get_by_id(&result_resource, id_to_find);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(result_resource != nullptr);

	REQUIRE(result_resource->id == 1);
	REQUIRE(result_resource->type == VACCEL_RES_SINGLE_MODEL);
	REQUIRE(result_resource->data == nullptr);
	REQUIRE_FALSE(list_empty(&result_resource->entry));
	REQUIRE(result_resource->refcount == 0);
	REQUIRE(result_resource->rundir == NULL);

	// Cleanup the test resource
	ret = resource_destroy(&test_res);
	REQUIRE(ret == VACCEL_OK);

	REQUIRE(test_res.id == 1);
	REQUIRE(test_res.type == VACCEL_RES_SINGLE_MODEL);
	REQUIRE(test_res.data == nullptr);
	REQUIRE(list_empty(&test_res.entry));
	REQUIRE(test_res.refcount == 0);
	REQUIRE(test_res.rundir == NULL);

	ret = resources_cleanup();
	REQUIRE(ret == VACCEL_OK);
}

TEST_CASE("with_deps", "[Resources]")
{
	int ret;
	struct vaccel_resource test_res;
	struct vaccel_resource test_dep_1;
	struct vaccel_resource test_dep_2;
	struct vaccel_resource **test_deps_g;
	struct vaccel_resource *test_deps[2] = { &test_dep_1, &test_dep_2 };
	size_t nr_deps;
	vaccel_resource_t test_type = VACCEL_RES_SHARED_OBJ;
	void *test_data = nullptr;
	int (*cleanup_res_test)(void *) = cleanup_resource_mock;

	// Ensure that the resource system is initialized
	ret = resources_bootstrap();
	REQUIRE(ret == VACCEL_OK);

	// Create a test resource
	ret = resource_new(&test_res, test_type, test_data, cleanup_res_test);
	REQUIRE(ret == VACCEL_OK);

	REQUIRE(test_res.id == 1);
	REQUIRE(test_res.type == VACCEL_RES_SHARED_OBJ);
	REQUIRE(test_res.data == nullptr);
	REQUIRE_FALSE(list_empty(&test_res.entry));
	REQUIRE(test_res.refcount == 0);
	REQUIRE(test_res.rundir == NULL);

	ret = resource_new(&test_dep_1, test_type, test_data, cleanup_res_test);
	REQUIRE(ret == VACCEL_OK);

	ret = resource_new(&test_dep_2, test_type, test_data, cleanup_res_test);
	REQUIRE(ret == VACCEL_OK);

	SECTION("valid_deps")
	{
		ret = resource_set_deps(&test_res, test_deps, 2);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(test_res.deps == test_deps);
		REQUIRE(test_res.nr_deps == 2);

		ret = vaccel_resource_get_deps(&test_deps_g, &nr_deps,
					       &test_res);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(test_deps_g == test_deps);
		REQUIRE(nr_deps == 2);

		ret = resource_unset_deps(&test_res);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(test_res.deps == NULL);
		REQUIRE(test_res.nr_deps == 0);

		vaccel_id_t dep_ids[2];
		ret = vaccel_resource_deps_to_ids(dep_ids, test_deps, 2);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(dep_ids[0] == 2);
		REQUIRE(dep_ids[1] == 3);

		ret = vaccel_resource_set_deps_from_ids(&test_res, dep_ids, 2);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(test_res.nr_deps == 2);
	}

	SECTION("null_values")
	{
		ret = resource_set_deps(&test_res, nullptr, 2);
		REQUIRE(ret == VACCEL_EINVAL);

		ret = resource_set_deps(&test_res, test_deps, 0);
		REQUIRE(ret == VACCEL_EINVAL);

		ret = resource_set_deps(nullptr, test_deps, 2);
		REQUIRE(ret == VACCEL_EINVAL);

		ret = vaccel_resource_get_deps(nullptr, &nr_deps, &test_res);
		REQUIRE(ret == VACCEL_EINVAL);

		ret = vaccel_resource_get_deps(&test_deps_g, nullptr,
					       &test_res);
		REQUIRE(ret == VACCEL_EINVAL);

		ret = vaccel_resource_get_deps(&test_deps_g, &nr_deps, nullptr);
		REQUIRE(ret == VACCEL_EINVAL);

		ret = resource_unset_deps(nullptr);
		REQUIRE(ret == VACCEL_EINVAL);

		vaccel_id_t dep_ids[2];
		ret = vaccel_resource_deps_to_ids(nullptr, test_deps, 2);
		REQUIRE(ret == VACCEL_EINVAL);

		ret = vaccel_resource_deps_to_ids(dep_ids, nullptr, 2);
		REQUIRE(ret == VACCEL_EINVAL);

		ret = vaccel_resource_deps_to_ids(dep_ids, test_deps, 0);
		REQUIRE(ret == VACCEL_EINVAL);

		ret = vaccel_resource_set_deps_from_ids(nullptr, dep_ids, 2);
		REQUIRE(ret == VACCEL_EINVAL);

		ret = vaccel_resource_set_deps_from_ids(&test_res, nullptr, 2);
		REQUIRE(ret == VACCEL_EINVAL);

		ret = vaccel_resource_set_deps_from_ids(&test_res, dep_ids, 0);
		REQUIRE(ret == VACCEL_EINVAL);
	}

	// Cleanup the test resource
	ret = resource_destroy(&test_dep_1);
	REQUIRE(ret == VACCEL_OK);

	REQUIRE(test_dep_1.data == nullptr);
	REQUIRE(list_empty(&test_dep_1.entry));
	REQUIRE(test_dep_1.refcount == 0);
	REQUIRE(test_dep_1.rundir == NULL);

	ret = resource_destroy(&test_dep_2);
	REQUIRE(ret == VACCEL_OK);

	REQUIRE(test_dep_2.data == nullptr);
	REQUIRE(list_empty(&test_dep_2.entry));
	REQUIRE(test_dep_2.refcount == 0);
	REQUIRE(test_dep_2.rundir == NULL);

	ret = resource_unset_deps(&test_res);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(test_res.deps == NULL);
	REQUIRE(test_res.nr_deps == 0);

	ret = resource_destroy(&test_res);
	REQUIRE(ret == VACCEL_OK);

	REQUIRE(test_res.data == nullptr);
	REQUIRE(list_empty(&test_res.entry));
	REQUIRE(test_res.refcount == 0);
	REQUIRE(test_res.rundir == NULL);

	ret = resources_cleanup();
	REQUIRE(ret == VACCEL_OK);
}

TEST_CASE("initialising with no resources bootstrapped")
{
	int ret;
	struct vaccel_resource test_res;
	vaccel_resource_t test_type = VACCEL_RES_SINGLE_MODEL;
	void *test_data = nullptr;
	int (*cleanup_res_test)(void *) = cleanup_resource_mock;
	struct vaccel_resource *result_resource = nullptr;
	vaccel_id_t id_to_find = 1;

	ret = resource_new(&test_res, test_type, test_data, cleanup_res_test);
	REQUIRE(ret == VACCEL_EPERM);

	ret = resource_get_by_id(&result_resource, id_to_find);
	REQUIRE(ret == VACCEL_EPERM);

	ret = resource_destroy(&test_res);
	REQUIRE(ret == VACCEL_EPERM);

	ret = resource_create_rundir(nullptr);
	REQUIRE(ret == VACCEL_EINVAL);

	ret = resources_cleanup();
	REQUIRE(ret == VACCEL_OK);
}
