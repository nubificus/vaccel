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
		ret = vaccel_resource_new(nullptr, nullptr, test_type);
		REQUIRE(ret == VACCEL_EINVAL);

		ret = vaccel_resource_destroy(nullptr);
		REQUIRE(ret == VACCEL_EINVAL);
	}

	// Test creation and destruction of a valid resource
	SECTION("Valid resource")
	{
		ret = vaccel_resource_new(&res, test_path, test_type);
		REQUIRE(ret == VACCEL_OK);

		REQUIRE(res.id == 1);
		REQUIRE(res.type == VACCEL_RESOURCE_LIB);
		REQUIRE_FALSE(list_empty(&res.entry));
		REQUIRE(res.refcount == 0);
		REQUIRE(res.rundir == NULL);

		ret = vaccel_resource_destroy(&res);
		REQUIRE(ret == VACCEL_OK);

		REQUIRE(res.id == 1);
		REQUIRE(res.type == VACCEL_RESOURCE_LIB);
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
	ret = vaccel_resource_new(&res, test_path, test_type);
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
	ret = vaccel_resource_destroy(&res);
	REQUIRE(ret == VACCEL_OK);

	REQUIRE(res.id == 1);
	REQUIRE(res.type == VACCEL_RESOURCE_LIB);
	REQUIRE(list_empty(&res.entry));
	REQUIRE(res.refcount == 0);
	REQUIRE_FALSE(res.rundir == NULL);

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
	ret = vaccel_resource_new(&test_res, test_path, test_type);
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
	ret = vaccel_resource_destroy(&test_res);
	REQUIRE(ret == VACCEL_OK);

	REQUIRE(test_res.id == 1);
	REQUIRE(test_res.type == VACCEL_RESOURCE_LIB);
	REQUIRE(list_empty(&test_res.entry));
	REQUIRE(test_res.refcount == 0);
	REQUIRE(test_res.rundir == NULL);

	free(test_path);
}

TEST_CASE("resource_with_deps", "[resources]")
{
	int ret;
	struct vaccel_resource test_res;
	struct vaccel_resource test_dep_1;
	struct vaccel_resource test_dep_2;
	struct vaccel_resource **test_deps_g;
	struct vaccel_resource *test_deps[2] = { &test_dep_1, &test_dep_2 };
	size_t nr_deps;
	vaccel_resource_t test_type = VACCEL_RESOURCE_LIB;
	char *test_path = abs_path(BUILD_ROOT, "examples/libmytestlib.so");

	// Create a test resource
	ret = vaccel_resource_new(&test_res, test_path, test_type);
	REQUIRE(ret == VACCEL_OK);

	REQUIRE(test_res.id == 1);
	REQUIRE(test_res.type == VACCEL_RESOURCE_LIB);
	REQUIRE_FALSE(list_empty(&test_res.entry));
	REQUIRE(test_res.refcount == 0);
	REQUIRE(test_res.rundir == NULL);

	ret = vaccel_resource_new(&test_dep_1, test_path, test_type);
	REQUIRE(ret == VACCEL_OK);

	ret = vaccel_resource_new(&test_dep_2, test_path, test_type);
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

		free(test_res.deps);
		ret = resource_unset_deps(&test_res);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(test_res.deps == NULL);
		REQUIRE(test_res.nr_deps == 0);
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
	ret = vaccel_resource_destroy(&test_dep_1);
	REQUIRE(ret == VACCEL_OK);

	REQUIRE(list_empty(&test_dep_1.entry));
	REQUIRE(test_dep_1.refcount == 0);
	REQUIRE(test_dep_1.rundir == NULL);

	ret = vaccel_resource_destroy(&test_dep_2);
	REQUIRE(ret == VACCEL_OK);

	REQUIRE(list_empty(&test_dep_2.entry));
	REQUIRE(test_dep_2.refcount == 0);
	REQUIRE(test_dep_2.rundir == NULL);

	ret = vaccel_resource_destroy(&test_res);
	REQUIRE(ret == VACCEL_OK);

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

	ret = vaccel_resource_new(&test_res, test_path, test_type);
	REQUIRE(ret == VACCEL_EPERM);

	ret = vaccel_resource_get_by_id(&result_resource, id_to_find);
	REQUIRE(ret == VACCEL_EPERM);

	ret = vaccel_resource_destroy(&test_res);
	REQUIRE(ret == VACCEL_EPERM);

	ret = resource_create_rundir(nullptr);
	REQUIRE(ret == VACCEL_EINVAL);

	// bootstrap again so the rest of the tests run correctly
	ret = resources_bootstrap();
	REQUIRE(ret == VACCEL_OK);

	free(test_path);
}
