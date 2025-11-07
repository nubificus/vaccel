// SPDX-License-Identifier: Apache-2.0

/*
 * The code below performs unit testing to `path` functions.
 *
 * 1) path_init_from_uri()
 * 2) path_from_uri()
 * 3) path_is_url()
 * 4) path_type()
 * 5) path_init_from_parts()
 * 6) path_from_parts()
 * 7) path_file_name()
 * 8) path_file_name_add_random_suffix()
 *
 */

#include "utils.hpp"
#include "vaccel.h"
#include <catch2/catch_test_macros.hpp>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <linux/limits.h>

TEST_CASE("path_from_uri", "[utils][path]")
{
	int ret;
	char path[PATH_MAX];
	char *alloc_path;
	char file[PATH_MAX];
	char dir[PATH_MAX];
	char u_file[PATH_MAX];
	char u_dir[PATH_MAX];
	const char *url = "http://nubificus.co.uk";
	vaccel_path_type_t path_type;

	ret = path_init_from_parts(file, PATH_MAX, BUILD_ROOT,
				   "examples/libmytestlib.so", nullptr);
	REQUIRE(ret == VACCEL_OK);
	ret = path_init_from_parts(dir, PATH_MAX, BUILD_ROOT, "examples",
				   nullptr);
	REQUIRE(ret == VACCEL_OK);
	ret = path_init_from_parts(u_file, PATH_MAX, "file:/", file, nullptr);
	REQUIRE(ret == VACCEL_OK);
	ret = path_init_from_parts(u_dir, PATH_MAX, "file:/", dir, nullptr);
	REQUIRE(ret == VACCEL_OK);

	SECTION("init success")
	{
		ret = path_init_from_uri(path, PATH_MAX, &path_type, file);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(strcmp(path, file) == 0);
		REQUIRE(path_type == VACCEL_PATH_LOCAL_FILE);

		ret = path_init_from_uri(path, PATH_MAX, &path_type, u_file);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(strcmp(path, file) == 0);
		REQUIRE(path_type == VACCEL_PATH_LOCAL_FILE);

		ret = path_init_from_uri(path, PATH_MAX, &path_type, dir);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(strcmp(path, dir) == 0);
		REQUIRE(path_type == VACCEL_PATH_LOCAL_DIR);

		ret = path_init_from_uri(path, PATH_MAX, &path_type, u_dir);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(strcmp(path, dir) == 0);
		REQUIRE(path_type == VACCEL_PATH_LOCAL_DIR);

		ret = path_init_from_uri(path, PATH_MAX, &path_type, url);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(strcmp(path, url) == 0);
		REQUIRE(path_type == VACCEL_PATH_REMOTE_FILE);
	}

	SECTION("init invalid arguments")
	{
		ret = path_init_from_uri(nullptr, PATH_MAX, nullptr, file);
		REQUIRE(ret == VACCEL_EINVAL);

		ret = path_init_from_uri(path, PATH_MAX, &path_type, nullptr);
		REQUIRE(ret == VACCEL_EINVAL);

		ret = path_init_from_uri(path, 0, &path_type, file);
		REQUIRE(ret == VACCEL_EINVAL);

		ret = path_init_from_uri(path, 10, &path_type, file);
		REQUIRE(ret == VACCEL_ENAMETOOLONG);
	}

	SECTION("alloc success")
	{
		ret = path_from_uri(&alloc_path, &path_type, file);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(strcmp(alloc_path, file) == 0);
		REQUIRE(path_type == VACCEL_PATH_LOCAL_FILE);
		free(alloc_path);

		ret = path_from_uri(&alloc_path, &path_type, u_file);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(strcmp(alloc_path, file) == 0);
		REQUIRE(path_type == VACCEL_PATH_LOCAL_FILE);
		free(alloc_path);

		ret = path_from_uri(&alloc_path, &path_type, dir);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(strcmp(alloc_path, dir) == 0);
		REQUIRE(path_type == VACCEL_PATH_LOCAL_DIR);
		free(alloc_path);

		ret = path_from_uri(&alloc_path, &path_type, u_dir);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(strcmp(alloc_path, dir) == 0);
		REQUIRE(path_type == VACCEL_PATH_LOCAL_DIR);
		free(alloc_path);

		ret = path_from_uri(&alloc_path, &path_type, url);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(strcmp(alloc_path, url) == 0);
		REQUIRE(path_type == VACCEL_PATH_REMOTE_FILE);
		free(alloc_path);
	}

	SECTION("alloc invalid arguments")
	{
		ret = path_from_uri(nullptr, nullptr, file);
		REQUIRE(ret == VACCEL_EINVAL);

		ret = path_from_uri(&alloc_path, &path_type, nullptr);
		REQUIRE(ret == VACCEL_EINVAL);
	}
}

TEST_CASE("path_type", "[utils][path]")
{
	int ret;
	char file[PATH_MAX];
	char dir[PATH_MAX];
	const char *url = "http://nubificus.co.uk";
	vaccel_path_type_t type;

	ret = path_init_from_parts(file, PATH_MAX, BUILD_ROOT,
				   "examples/libmytestlib.so", nullptr);
	REQUIRE(ret == VACCEL_OK);
	ret = path_init_from_parts(dir, PATH_MAX, BUILD_ROOT, "examples",
				   nullptr);
	REQUIRE(ret == VACCEL_OK);

	SECTION("success")
	{
		ret = path_type(file, &type);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(type == VACCEL_PATH_LOCAL_FILE);

		ret = path_type(dir, &type);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(type == VACCEL_PATH_LOCAL_DIR);

		ret = path_type(url, &type);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(type == VACCEL_PATH_REMOTE_FILE);
	}

	SECTION("invalid arguments")
	{
		ret = path_type(nullptr, &type);
		REQUIRE(ret == VACCEL_EINVAL);

		ret = path_type(file, nullptr);
		REQUIRE(ret == VACCEL_EINVAL);

		const char *word = "invalid";
		ret = path_type(word, &type);
		REQUIRE(ret == VACCEL_ENOTSUP);
	}
}

TEST_CASE("path_is_url", "[utils][path]")
{
	const char *url = "http://nubificus.co.uk";
	const char *word = "hello world";

	SECTION("URL case")
	{
		REQUIRE(path_is_url(url));
	}

	SECTION("Non URL case")
	{
		REQUIRE(path_is_url(word) == false);
	}
}

TEST_CASE("path_from_parts", "[utils][path]")
{
	int ret;

	SECTION("init")
	{
		char buf[64] = { '\0' };

		ret = path_init_from_parts(nullptr, 0, nullptr);
		REQUIRE(ret == VACCEL_EINVAL);

		ret = path_init_from_parts(buf, PATH_MAX + 1, "one", "two",
					   nullptr);
		REQUIRE(ret == VACCEL_ENAMETOOLONG);

		ret = path_init_from_parts(buf, 10, "longer_than_10", "two",
					   "three", nullptr);
		REQUIRE(ret == VACCEL_ENAMETOOLONG);

		ret = path_init_from_parts(buf, 5, "one", "two", "three",
					   nullptr);
		REQUIRE(ret == VACCEL_ENAMETOOLONG);

		ret = path_init_from_parts(buf, sizeof(buf), "one", "two",
					   "three", nullptr);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(strcmp(buf, "one/two/three") == 0);
	}

	SECTION("with memory allocation")
	{
		char *buf;

		ret = path_from_parts(nullptr, "first", nullptr);
		REQUIRE(ret == VACCEL_EINVAL);
		ret = path_from_parts(&buf, nullptr);
		REQUIRE(ret == VACCEL_EINVAL);

		char one[PATH_MAX - 1];
		char two[PATH_MAX - 1];

		memset(one, 'a', sizeof(one) - 1);
		one[PATH_MAX - 2] = '\0';

		memset(two, 'b', sizeof(two) - 1);
		two[PATH_MAX - 2] = '\0';

		ret = path_from_parts(&buf, one, two, nullptr);
		REQUIRE(ret == VACCEL_ENAMETOOLONG);

		ret = path_from_parts(&buf, "one", "two", "three", nullptr);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(strcmp(buf, "one/two/three") == 0);
		free(buf);
	}
}

TEST_CASE("path_file_name", "[utils][path]")
{
	char name[NAME_MAX];
	char *alloc_name = nullptr;
	const char *filepath = "/path/to/file";
	int ret;

	SECTION("NULL arguments")
	{
		ret = path_file_name(nullptr, name, NAME_MAX, &alloc_name);
		REQUIRE(ret == VACCEL_EINVAL);
		ret = path_file_name(nullptr, name, NAME_MAX, nullptr);
		REQUIRE(ret == VACCEL_EINVAL);
		ret = path_file_name(nullptr, nullptr, NAME_MAX, &alloc_name);
		REQUIRE(ret == VACCEL_EINVAL);
		ret = path_file_name(nullptr, name, 0, &alloc_name);
		REQUIRE(ret == VACCEL_EINVAL);
		ret = path_file_name(filepath, nullptr, NAME_MAX, nullptr);
		REQUIRE(ret == VACCEL_EINVAL);
		ret = path_file_name(filepath, name, 0, nullptr);
		REQUIRE(ret == VACCEL_EINVAL);
		ret = path_file_name(filepath, nullptr, 0, nullptr);
		REQUIRE(ret == VACCEL_EINVAL);
		ret = path_file_name(nullptr, nullptr, 0, nullptr);
		REQUIRE(ret == VACCEL_EINVAL);
	}

	SECTION("name too long")
	{
		char name_toolong[NAME_MAX + 1];
		memset(name_toolong, 'a', NAME_MAX);
		name_toolong[NAME_MAX] = '\0';

		char *fullpath;
		ret = path_from_parts(&fullpath, "dir", name_toolong, nullptr);
		REQUIRE(ret == VACCEL_OK);

		ret = path_file_name(fullpath, nullptr, 0, &alloc_name);
		REQUIRE(ret == VACCEL_ENAMETOOLONG);
		REQUIRE(alloc_name == nullptr);
		free(fullpath);
	}

	SECTION("size too small")
	{
		ret = path_file_name(filepath, name, 1, nullptr);
		REQUIRE(ret == VACCEL_ENAMETOOLONG);
	}

	SECTION("success")
	{
		ret = path_file_name(filepath, name, NAME_MAX, nullptr);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(strcmp(name, "file") == 0);

		ret = path_file_name(filepath, nullptr, 0, &alloc_name);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(strcmp(name, "file") == 0);
		free(alloc_name);
	}
}

TEST_CASE("path_file_name_add_random_suffix", "[utils][path]")
{
	char buf[PATH_MAX] = { '\0' };
	size_t ext_len;
	int ret;

	SECTION("NULL Arguments")
	{
		ret = path_file_name_add_random_suffix(nullptr, PATH_MAX,
						       &ext_len,
						       "path/to/file.txt",
						       "suffix");
		REQUIRE(ret == VACCEL_EINVAL);

		ret = path_file_name_add_random_suffix(
			buf, 0, &ext_len, "path/to/file.txt", "suffix");
		REQUIRE(ret == VACCEL_EINVAL);

		ret = path_file_name_add_random_suffix(
			buf, PATH_MAX, nullptr, "path/to/file.txt", "suffix");
		REQUIRE(ret == VACCEL_EINVAL);

		ret = path_file_name_add_random_suffix(buf, PATH_MAX, &ext_len,
						       nullptr, "suffix");
		REQUIRE(ret == VACCEL_EINVAL);

		ret = path_file_name_add_random_suffix(
			buf, PATH_MAX, &ext_len, "path/to/file.txt", nullptr);
		REQUIRE(ret == VACCEL_EINVAL);
	}

	SECTION("OK with suffix")
	{
		ret = path_file_name_add_random_suffix(
			buf, PATH_MAX, &ext_len, "path/to/file.txt", "suffix");
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(ext_len == 4);
		REQUIRE(strcmp(buf, "path/to/filesuffix.txt") == 0);
	}

	SECTION("Size too small")
	{
		ret = path_file_name_add_random_suffix(
			buf, 1, &ext_len, "path/to/file.txt", "suffix");
		REQUIRE(ret == VACCEL_ENAMETOOLONG);
	}

	SECTION("No suffix")
	{
		memset(buf, 0, PATH_MAX);
		ret = path_file_name_add_random_suffix(
			buf, PATH_MAX, &ext_len, "path/to/file", "suffix");
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(ext_len == 0);
		REQUIRE(strcmp(buf, "path/to/filesuffix") == 0);
	}
}
