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
#include <unistd.h>
#include <utils.hpp>
#include <vaccel.h>

int process_files_callback(const char *path, int idx, va_list args)
{
	char **paths = va_arg(args, char **);
	if (paths == nullptr)
		return VACCEL_EINVAL;

	paths[idx] = strdup(path);
	return VACCEL_OK;
}

TEST_CASE("fs_path_exists", "[utils][fs]")
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

	SECTION("Invalid arguments")
	{
		ret = fs_path_exists(nullptr);
		REQUIRE(ret == false);
	}

	free(existing_file);
	free(non_existing_file);
	free(existing_dir);
	free(non_existing_dir);
}

TEST_CASE("fs_path_is_dir", "[utils][fs]")
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

	SECTION("Invalid arguments")
	{
		ret = fs_path_is_dir(nullptr);
		REQUIRE(ret == false);
	}

	free(existing_file);
	free(non_existing_file);
	free(existing_dir);
	free(non_existing_dir);
}

TEST_CASE("fs_path_is_file", "[utils][fs]")
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

	SECTION("Invalid arguments")
	{
		ret = fs_path_is_file(nullptr);
		REQUIRE(ret == false);
	}

	free(existing_file);
	free(non_existing_file);
	free(existing_dir);
	free(non_existing_dir);
}

TEST_CASE("fs_ops", "[utils][fs]")
{
	char rootpath[PATH_MAX];
	char dirpath[PATH_MAX];
	char filepath1[PATH_MAX];
	char filepath2[PATH_MAX];

	int ret = path_init_from_parts(rootpath, PATH_MAX, vaccel_rundir(),
				       "test", nullptr);
	REQUIRE(ret == VACCEL_OK);
	ret = path_init_from_parts(dirpath, PATH_MAX, rootpath, "test_dir1",
				   nullptr);
	REQUIRE(ret == VACCEL_OK);
	ret = path_init_from_parts(filepath1, PATH_MAX, dirpath, "test_file1",
				   nullptr);
	REQUIRE(ret == VACCEL_OK);
	ret = path_init_from_parts(filepath2, PATH_MAX, dirpath, "test_file2",
				   nullptr);
	REQUIRE(ret == VACCEL_OK);

	SECTION("Create directory")
	{
		ret = fs_dir_create(dirpath);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(fs_path_is_dir(dirpath));
	}

	SECTION("Create existing directory")
	{
		REQUIRE(fs_dir_create(dirpath) == VACCEL_EEXIST);
	}

	SECTION("Number of files (0)")
	{
		REQUIRE(fs_dir_process_files(dirpath, nullptr) == 0);
	}

	SECTION("Create files")
	{
		REQUIRE(fs_file_create(filepath1, nullptr) == VACCEL_OK);
		REQUIRE(fs_path_is_file(filepath1));
		int fd;
		REQUIRE(fs_file_create(filepath2, &fd) == VACCEL_OK);
		REQUIRE(fd > 0);
		REQUIRE(fs_path_is_file(filepath2));
		close(fd);
	}

	SECTION("Create existing file")
	{
		REQUIRE(fs_file_create(filepath1, nullptr) == VACCEL_EEXIST);
	}

	SECTION("Number of files (2)")
	{
		REQUIRE(fs_dir_process_files(dirpath, nullptr) == 2);
	}

	SECTION("Get dir file paths (2)")
	{
		char *paths[2];
		for (auto &path : paths)
			path = nullptr;
		ret = fs_dir_process_files(dirpath, process_files_callback,
					   paths);
		REQUIRE(ret == 2);
		for (auto &path : paths) {
			REQUIRE(path != nullptr);
			REQUIRE((strcmp(path, filepath1) == 0 ||
				 strcmp(path, filepath2) == 0));
			free(path);
		}
	}

	SECTION("Remove files")
	{
		REQUIRE(fs_file_remove(filepath1) == VACCEL_OK);
		REQUIRE(fs_path_is_file(filepath1) == false);
		REQUIRE(fs_file_remove(filepath2) == VACCEL_OK);
		REQUIRE(fs_path_is_file(filepath2) == false);
	}

	SECTION("Number of files (0)")
	{
		REQUIRE(fs_dir_process_files(dirpath, nullptr) == 0);
	}

	SECTION("Create file - unique")
	{
		char *final_path;
		ret = fs_file_create_unique(filepath1, 0, &final_path, nullptr);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(fs_path_is_file(final_path));
		REQUIRE(fs_file_remove(final_path) == VACCEL_OK);
		free(final_path);

		ret = fs_file_create_unique(filepath1, PATH_MAX, nullptr,
					    nullptr);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(fs_path_is_file(filepath1));
		REQUIRE(fs_file_remove(filepath1) == VACCEL_OK);
	}

	SECTION("Invalid arguments")
	{
		REQUIRE(fs_file_create(filepath1, nullptr) == VACCEL_OK);

		char *final_path;
		char toolong_path[PATH_MAX + 1] = { '\0' };
		memset(toolong_path, 'a', sizeof(toolong_path) - 1);
		REQUIRE(fs_dir_create(nullptr) == VACCEL_EINVAL);
		REQUIRE(fs_dir_create(toolong_path) == VACCEL_ENAMETOOLONG);
		REQUIRE(fs_dir_create(filepath1) == VACCEL_ENOTDIR);

		REQUIRE(fs_dir_create_unique(nullptr, PATH_MAX, &final_path) ==
			VACCEL_EINVAL);
		REQUIRE(fs_dir_create_unique(dirpath, 0, nullptr) ==
			VACCEL_EINVAL);
		REQUIRE(fs_dir_create_unique(toolong_path, PATH_MAX, nullptr) ==
			VACCEL_ENAMETOOLONG);

		REQUIRE(fs_file_create(nullptr, nullptr) == VACCEL_EINVAL);
		REQUIRE(fs_file_create(toolong_path, nullptr) ==
			VACCEL_ENAMETOOLONG);
		REQUIRE(fs_file_create(dirpath, nullptr) == VACCEL_EINVAL);

		REQUIRE(fs_file_create_unique(nullptr, 0, &final_path,
					      nullptr) == VACCEL_EINVAL);
		REQUIRE(fs_file_create_unique(filepath1, 0, nullptr, nullptr) ==
			VACCEL_EINVAL);
		REQUIRE(fs_file_create_unique(toolong_path, PATH_MAX, nullptr,
					      nullptr) == VACCEL_ENAMETOOLONG);

		REQUIRE(fs_file_remove(nullptr) == VACCEL_EINVAL);

		REQUIRE(fs_dir_remove(nullptr) == VACCEL_EINVAL);

		REQUIRE(fs_dir_process_files(nullptr, nullptr) ==
			-VACCEL_EINVAL);

		REQUIRE(fs_file_remove(filepath1) == VACCEL_OK);
	}

	SECTION("Remove directory")
	{
		REQUIRE(fs_dir_remove(dirpath) == VACCEL_OK);
		REQUIRE(fs_path_is_dir(dirpath) == false);
		REQUIRE(fs_dir_remove(rootpath) == VACCEL_OK);
		REQUIRE(fs_path_is_dir(rootpath) == false);
	}

	SECTION("Create directory - unique")
	{
		char *final_dir;
		ret = fs_dir_create_unique(dirpath, 0, &final_dir);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(fs_path_is_dir(final_dir));
		REQUIRE(fs_dir_remove(final_dir) == VACCEL_OK);
		free(final_dir);

		ret = fs_dir_create_unique(dirpath, PATH_MAX, nullptr);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(fs_path_is_dir(dirpath));
		REQUIRE(fs_dir_remove(dirpath) == VACCEL_OK);
	}
}

TEST_CASE("fs_file_read", "[utils][fs]")
{
	char *existing_file = abs_path(BUILD_ROOT, "examples/libmytestlib.so");
	int ret;
	unsigned char *mmap_handle;
	unsigned char *std_handle;
	size_t mmap_size;
	size_t std_size;

	ret = fs_file_read(existing_file, (void **)&std_handle, &std_size);
	REQUIRE(ret == VACCEL_OK);

	ret = fs_file_read_mmap(existing_file, (void **)&mmap_handle,
				&mmap_size);
	REQUIRE(ret == VACCEL_OK);

	REQUIRE(std_size == mmap_size);

	for (size_t i = 0; i < std_size; i++) {
		REQUIRE(std_handle[i] == mmap_handle[i]);
	}
	REQUIRE(munmap(mmap_handle, mmap_size) == 0);

	SECTION("Invalid arguments")
	{
		ret = fs_file_read(nullptr, (void **)&std_handle, &std_size);
		REQUIRE(ret == VACCEL_EINVAL);

		ret = fs_file_read(existing_file, nullptr, &std_size);
		REQUIRE(ret == VACCEL_EINVAL);

		ret = fs_file_read_mmap(nullptr, (void **)&mmap_handle,
					&mmap_size);
		REQUIRE(ret == VACCEL_EINVAL);

		ret = fs_file_read_mmap(existing_file, nullptr, &mmap_size);
		REQUIRE(ret == VACCEL_EINVAL);
	}

	free(std_handle);
	free(existing_file);
}
