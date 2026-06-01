// SPDX-License-Identifier: Apache-2.0

/*
 * Unit Testing for VAccel Profiling
 */

#include "vaccel.h"
#include <catch2/catch_test_macros.hpp>
#include <cstdlib>
#include <cstring>
#include <thread>
#include <vector>

TEST_CASE("vaccel_prof_enabled", "[prof]")
{
	REQUIRE(vaccel_prof_enabled());
}

TEST_CASE("vaccel_prof_flush", "[prof]")
{
	REQUIRE(vaccel_prof_flush() == VACCEL_OK);
}

TEST_CASE("vaccel_prof_region_init", "[prof]")
{
	int ret;
	struct vaccel_prof_region region;

	SECTION("success")
	{
		ret = vaccel_prof_region_init(&region, "test_region");
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(region.name != nullptr);
		REQUIRE(strcmp(region.name, "test_region") == 0);
		REQUIRE(region.name_owned == true);
		REQUIRE(region.nr_entries == 0);
		REQUIRE(region.samples != nullptr);
		REQUIRE(region.size > 0);

		REQUIRE(vaccel_prof_region_release(&region) == VACCEL_OK);
	}

	SECTION("null name")
	{
		ret = vaccel_prof_region_init(&region, nullptr);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(region.name != nullptr);
		REQUIRE(region.name[0] == '\0');
		REQUIRE(region.name_owned == true);

		REQUIRE(vaccel_prof_region_release(&region) == VACCEL_OK);
	}

	SECTION("invalid arguments")
	{
		ret = vaccel_prof_region_init(nullptr, "test_region");
		REQUIRE(ret == VACCEL_EINVAL);
	}
}

TEST_CASE("vaccel_prof_region_release", "[prof]")
{
	int ret;
	struct vaccel_prof_region region;

	SECTION("success")
	{
		REQUIRE(vaccel_prof_region_init(&region, "test_region") ==
			VACCEL_OK);

		ret = vaccel_prof_region_release(&region);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(region.name == nullptr);
		REQUIRE(region.name_owned == false);
		REQUIRE(region.samples == nullptr);
		REQUIRE(region.size == 0);
		REQUIRE(region.nr_entries == 0);
	}

	SECTION("invalid arguments")
	{
		ret = vaccel_prof_region_release(nullptr);
		REQUIRE(ret == VACCEL_EINVAL);
	}
}

TEST_CASE("vaccel_prof_region_start", "[prof]")
{
	int ret;
	struct vaccel_prof_region region;

	SECTION("success")
	{
		REQUIRE(vaccel_prof_region_init(&region, "test_region") ==
			VACCEL_OK);

		ret = vaccel_prof_region_start(&region);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(region.nr_entries == 1);

		REQUIRE(vaccel_prof_region_start(&region) == VACCEL_OK);
		REQUIRE(region.nr_entries == 2);

		REQUIRE(vaccel_prof_region_release(&region) == VACCEL_OK);
	}

	SECTION("invalid arguments")
	{
		ret = vaccel_prof_region_start(nullptr);
		REQUIRE(ret == VACCEL_EINVAL);
	}
}

TEST_CASE("vaccel_prof_region_stop", "[prof]")
{
	int ret;
	struct vaccel_prof_region region;

	SECTION("success")
	{
		REQUIRE(vaccel_prof_region_init(&region, "test_region") ==
			VACCEL_OK);
		REQUIRE(vaccel_prof_region_start(&region) == VACCEL_OK);

		ret = vaccel_prof_region_stop(&region);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(region.nr_entries == 1);

		REQUIRE(vaccel_prof_region_release(&region) == VACCEL_OK);
	}

	SECTION("stop before start")
	{
		REQUIRE(vaccel_prof_region_init(&region, "test_region") ==
			VACCEL_OK);

		ret = vaccel_prof_region_stop(&region);
		REQUIRE(ret == VACCEL_ENOENT);

		REQUIRE(vaccel_prof_region_release(&region) == VACCEL_OK);
	}

	SECTION("invalid arguments")
	{
		ret = vaccel_prof_region_stop(nullptr);
		REQUIRE(ret == VACCEL_EINVAL);
	}
}

TEST_CASE("vaccel_prof_region_stop_with_context", "[prof]")
{
	int ret;
	struct vaccel_prof_region region;

	/*
	 * The base backend leaves region_stop_with_context unset, so the call
	 * falls back to region_stop and op_type/plugin_name are dropped here.
	 * Verifying that those arguments are forwarded to a backend that does
	 * implement the hook is covered by integration tests against a real
	 * backend.
	 */
	SECTION("success")
	{
		REQUIRE(vaccel_prof_region_init(&region, "test_region") ==
			VACCEL_OK);
		REQUIRE(vaccel_prof_region_start(&region) == VACCEL_OK);

		ret = vaccel_prof_region_stop_with_context(
			&region, VACCEL_OP_NOOP, "noop");
		REQUIRE(ret == VACCEL_OK);

		REQUIRE(vaccel_prof_region_release(&region) == VACCEL_OK);
	}

	SECTION("invalid arguments")
	{
		ret = vaccel_prof_region_stop_with_context(
			nullptr, VACCEL_OP_NOOP, "noop");
		REQUIRE(ret == VACCEL_EINVAL);
	}
}

TEST_CASE("vaccel_prof_region_print", "[prof]")
{
	int ret;
	struct vaccel_prof_region region;

	SECTION("success")
	{
		REQUIRE(vaccel_prof_region_init(&region, "test_region") ==
			VACCEL_OK);

		ret = vaccel_prof_region_print(&region);
		REQUIRE(ret == VACCEL_OK);

		REQUIRE(vaccel_prof_region_release(&region) == VACCEL_OK);
	}

	SECTION("invalid arguments")
	{
		ret = vaccel_prof_region_print(nullptr);
		REQUIRE(ret == VACCEL_EINVAL);
	}
}

TEST_CASE("vaccel_prof_regions_init", "[prof]")
{
	int ret;
	struct vaccel_prof_region regions[2];

	SECTION("success")
	{
		ret = vaccel_prof_regions_init(regions, 2);
		REQUIRE(ret == VACCEL_OK);

		REQUIRE(vaccel_prof_regions_release(regions, 2) == VACCEL_OK);
	}

	SECTION("invalid arguments")
	{
		ret = vaccel_prof_regions_init(nullptr, 2);
		REQUIRE(ret == VACCEL_EINVAL);
	}
}

TEST_CASE("vaccel_prof_regions_release", "[prof]")
{
	int ret;
	struct vaccel_prof_region regions[2];

	SECTION("success")
	{
		REQUIRE(vaccel_prof_regions_init(regions, 2) == VACCEL_OK);

		ret = vaccel_prof_regions_release(regions, 2);
		REQUIRE(ret == VACCEL_OK);
	}

	SECTION("invalid arguments")
	{
		ret = vaccel_prof_regions_release(nullptr, 2);
		REQUIRE(ret == VACCEL_EINVAL);
	}
}

TEST_CASE("vaccel_prof_regions_start_by_name", "[prof]")
{
	int ret;
	struct vaccel_prof_region regions[] = {
		VACCEL_PROF_REGION_INIT("region_0"),
		VACCEL_PROF_REGION_INIT("region_1"),
	};

	SECTION("success")
	{
		ret = vaccel_prof_regions_start_by_name(regions, 2, "region_0");
		REQUIRE(ret == VACCEL_OK);

		REQUIRE(vaccel_prof_regions_release(regions, 2) == VACCEL_OK);
	}

	SECTION("name not found")
	{
		ret = vaccel_prof_regions_start_by_name(regions, 2, "missing");
		REQUIRE(ret == VACCEL_EINVAL);
	}

	SECTION("invalid arguments")
	{
		ret = vaccel_prof_regions_start_by_name(nullptr, 2, "region_0");
		REQUIRE(ret == VACCEL_EINVAL);
	}
}

TEST_CASE("vaccel_prof_regions_stop_by_name", "[prof]")
{
	int ret;
	struct vaccel_prof_region regions[] = {
		VACCEL_PROF_REGION_INIT("region_0"),
		VACCEL_PROF_REGION_INIT("region_1"),
	};

	SECTION("success")
	{
		REQUIRE(vaccel_prof_regions_start_by_name(
				regions, 2, "region_0") == VACCEL_OK);

		ret = vaccel_prof_regions_stop_by_name(regions, 2, "region_0");
		REQUIRE(ret == VACCEL_OK);

		REQUIRE(vaccel_prof_regions_release(regions, 2) == VACCEL_OK);
	}

	SECTION("stop before start")
	{
		ret = vaccel_prof_regions_stop_by_name(regions, 2, "region_0");
		REQUIRE(ret == VACCEL_ENOENT);
	}

	SECTION("name not found")
	{
		ret = vaccel_prof_regions_stop_by_name(regions, 2, "missing");
		REQUIRE(ret == VACCEL_EINVAL);
	}

	SECTION("invalid arguments")
	{
		ret = vaccel_prof_regions_stop_by_name(nullptr, 2, "region_0");
		REQUIRE(ret == VACCEL_EINVAL);
	}
}

TEST_CASE("vaccel_prof_regions_print_all", "[prof]")
{
	int ret;
	struct vaccel_prof_region regions[] = {
		VACCEL_PROF_REGION_INIT("region_0"),
		VACCEL_PROF_REGION_INIT("region_1"),
	};

	SECTION("success")
	{
		ret = vaccel_prof_regions_print_all(regions, 2);
		REQUIRE(ret == VACCEL_OK);

		REQUIRE(vaccel_prof_regions_release(regions, 2) == VACCEL_OK);
	}

	SECTION("invalid arguments")
	{
		ret = vaccel_prof_regions_print_all(nullptr, 2);
		REQUIRE(ret == VACCEL_EINVAL);
	}
}

TEST_CASE("vaccel_prof_regions_print_all_to_buf", "[prof]")
{
	int ret;
	char *buf = nullptr;
	struct vaccel_prof_region regions[] = {
		VACCEL_PROF_REGION_INIT("region_0"),
		VACCEL_PROF_REGION_INIT("region_1"),
	};

	SECTION("success")
	{
		ret = vaccel_prof_regions_print_all_to_buf(&buf, 1024, regions,
							   2);
		REQUIRE(ret == 2);
		REQUIRE(buf != nullptr);

		free(buf);
		REQUIRE(vaccel_prof_regions_release(regions, 2) == VACCEL_OK);
	}

	SECTION("size query")
	{
		REQUIRE(vaccel_prof_regions_start_by_name(
				regions, 2, "region_0") == VACCEL_OK);
		REQUIRE(vaccel_prof_regions_stop_by_name(
				regions, 2, "region_0") == VACCEL_OK);

		ret = vaccel_prof_regions_print_all_to_buf(nullptr, 0, regions,
							   2);
		REQUIRE(ret > 0);

		REQUIRE(vaccel_prof_regions_release(regions, 2) == VACCEL_OK);
	}

	SECTION("invalid arguments")
	{
		ret = vaccel_prof_regions_print_all_to_buf(&buf, 1024, nullptr,
							   2);
		REQUIRE(ret == -VACCEL_EINVAL);
	}
}

enum {
	TEST_THREADS_NUM = 8,
	TEST_ITERATIONS_NUM = 5000,
};

TEST_CASE("prof_region_concurrent_start_stop", "[prof]")
{
	struct vaccel_prof_region region;
	REQUIRE(vaccel_prof_region_init(&region, "concurrent_region") ==
		VACCEL_OK);

	std::vector<std::thread> threads;
	for (int t = 0; t < TEST_THREADS_NUM; t++) {
		threads.emplace_back([&region]() {
			for (int i = 0; i < TEST_ITERATIONS_NUM; i++) {
				vaccel_prof_region_start(&region);
				vaccel_prof_region_stop(&region);
			}
		});
	}
	for (auto &thread : threads)
		thread.join();

	REQUIRE(region.nr_entries ==
		(size_t)TEST_THREADS_NUM * TEST_ITERATIONS_NUM);

	REQUIRE(vaccel_prof_region_release(&region) == VACCEL_OK);
}
