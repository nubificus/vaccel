// SPDX-License-Identifier: Apache-2.0

/*
 *
 * The code below performs unit testing to fs operations.
 *
 * 1)  fs_path_exists()
 * 2)  fs_path_is_dir()
 * 3)  fs_path_is_file()
 * 4)  fs_dir_num_files()
 * 5)  fs_dir_create()
 * 6)  fs_dir_remove()
 * 7)  fs_file_create()
 * 8)  fs_file_remove()
 * 9)  fs_file_read()
 * 10) fs_file_read_mmap()
 *
 */

#include <catch.hpp>
#include <sys/mman.h>
#include <utils.hpp>
#include <vaccel.h>

TEST_CASE("fs_path_exists", "[utils/fs]")
{
	char *existing_file = abs_path(BUILD_ROOT, "examples/libmytestlib.so");
	char *non_existing_file =
		abs_path(BUILD_ROOT, "examples/librandomlib.so");
	char *existing_dir = abs_path(BUILD_ROOT, "examples");
	char *non_existing_dir = abs_path(BUILD_ROOT, "exampless");
	bool ret;

	SECTION("Existing file")
	{
		ret = fs_path_exists(existing_file);
		REQUIRE(ret);
	}

	SECTION("Non existing file")
	{
		ret = fs_path_exists(non_existing_file);
		REQUIRE(ret == false);
	}

	SECTION("Existing directory")
	{
		ret = fs_path_exists(existing_dir);
		REQUIRE(ret);
	}

	SECTION("Non existing directory")
	{
		ret = fs_path_exists(non_existing_dir);
		REQUIRE(ret == false);
	}

	free(existing_file);
	free(non_existing_file);
	free(existing_dir);
	free(non_existing_dir);
}

TEST_CASE("fs_path_is_dir", "[utils/fs]")
{
	char *existing_file = abs_path(BUILD_ROOT, "examples/libmytestlib.so");
	char *non_existing_file =
		abs_path(BUILD_ROOT, "examples/librandomlib.so");
	char *existing_dir = abs_path(BUILD_ROOT, "examples");
	char *non_existing_dir = abs_path(BUILD_ROOT, "exampless");
	bool ret;

	SECTION("Existing file")
	{
		ret = fs_path_is_dir(existing_file);
		REQUIRE(ret == false);
	}

	SECTION("Non existing file")
	{
		ret = fs_path_is_dir(non_existing_file);
		REQUIRE(ret == false);
	}

	SECTION("Existing directory")
	{
		ret = fs_path_is_dir(existing_dir);
		REQUIRE(ret);
	}

	SECTION("Non existing directory")
	{
		ret = fs_path_is_dir(non_existing_dir);
		REQUIRE(ret == false);
	}

	free(existing_file);
	free(non_existing_file);
	free(existing_dir);
	free(non_existing_dir);
}

TEST_CASE("fs_path_is_file", "[utils/fs]")
{
	char *existing_file = abs_path(BUILD_ROOT, "examples/libmytestlib.so");
	char *non_existing_file =
		abs_path(BUILD_ROOT, "examples/librandomlib.so");
	char *existing_dir = abs_path(BUILD_ROOT, "examples");
	char *non_existing_dir = abs_path(BUILD_ROOT, "exampless");
	bool ret;

	SECTION("Existing file")
	{
		ret = fs_path_is_file(existing_file);
		REQUIRE(ret);
	}

	SECTION("Non existing file")
	{
		ret = fs_path_is_file(non_existing_file);
		REQUIRE(ret == false);
	}

	SECTION("Existing directory")
	{
		ret = fs_path_is_file(existing_dir);
		REQUIRE(ret == false);
	}

	SECTION("Non existing directory")
	{
		ret = fs_path_is_file(non_existing_dir);
		REQUIRE(ret == false);
	}

	free(existing_file);
	free(non_existing_file);
	free(existing_dir);
	free(non_existing_dir);
}

TEST_CASE("fs_ops", "[utils/fs]")
{
	char *dirname = abs_path(BUILD_ROOT, "my_dir1/my_dir2");
	char *filepath = abs_path(BUILD_ROOT, "my_dir1/my_dir2/my_file");
	int ret;

	SECTION("Create directory")
	{
		ret = fs_dir_create(dirname);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(fs_path_is_dir(dirname));
	}

	SECTION("Number of files (0)")
	{
		REQUIRE(0 == fs_dir_num_files(dirname));
	}

	SECTION("Create file")
	{
		REQUIRE(VACCEL_OK == fs_file_create(filepath, nullptr));
		REQUIRE(fs_path_is_file(filepath));
	}

	SECTION("Number of files (1)")
	{
		REQUIRE(1 == fs_dir_num_files(dirname));
	}

	SECTION("Remove file")
	{
		REQUIRE(VACCEL_OK == fs_file_remove(filepath));
		REQUIRE(false == fs_path_is_file(filepath));
	}

	SECTION("Number of files (0)")
	{
		REQUIRE(0 == fs_dir_num_files(dirname));
	}

	SECTION("Remove directory")
	{
		REQUIRE(VACCEL_OK == fs_dir_remove(dirname));
		REQUIRE(false == fs_path_is_dir(dirname));
	}

	free(dirname);
	free(filepath);
}

TEST_CASE("fs_file_reading", "[utils/fs]")
{
	char *existing_file = abs_path(BUILD_ROOT, "examples/libmytestlib.so");
	int ret;
	unsigned char *mmap_handle;
	unsigned char *std_handle;
	size_t mmap_size;
	size_t std_size;

	ret = fs_file_read(existing_file, (void **)&std_handle, &std_size);
	REQUIRE(0 == ret);

	ret = fs_file_read_mmap(existing_file, (void **)&mmap_handle,
				&mmap_size);
	REQUIRE(0 == ret);

	REQUIRE(std_size == mmap_size);

	for (size_t i = 0; i < std_size; i++) {
		REQUIRE(std_handle[i] == mmap_handle[i]);
	}
	REQUIRE(0 == munmap(mmap_handle, mmap_size));

	free(std_handle);
	free(existing_file);
}
