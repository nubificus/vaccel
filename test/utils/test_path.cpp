// SPDX-License-Identifier: Apache-2.0

/*
 *
 * The code below performs unit testing to `path` functions.
 *
 * 1) path_is_url()
 * 2) path_init_from_parts()
 * 3) path_from_parts()
 * 4) path_file_name()
 * 5) path_file_name_add_random_suffix()
 *
 */

#include <catch.hpp>
#include <iostream>
#include <utils.hpp>
#include <vaccel.h>

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
		REQUIRE(false == path_is_url(word));
	}
}

TEST_CASE("path_from_parts", "[utils][path]")
{
	int ret;

	SECTION("init")
	{
		char buf[64] = { 0 };

		REQUIRE(VACCEL_EINVAL ==
			path_init_from_parts(nullptr, 0, nullptr));

		ret = path_init_from_parts(buf, PATH_MAX + 1, "one", "two",
					   nullptr);
		REQUIRE(VACCEL_ENAMETOOLONG == ret);

		ret = path_init_from_parts(buf, 10, "longer_than_10", "two",
					   "three", nullptr);
		REQUIRE(VACCEL_ENAMETOOLONG == ret);

		ret = path_init_from_parts(buf, 5, "one", "two", "three",
					   nullptr);
		REQUIRE(VACCEL_ENAMETOOLONG == ret);

		ret = path_init_from_parts(buf, sizeof(buf), "one", "two",
					   "three", nullptr);
		REQUIRE(0 == ret);
		REQUIRE(0 == strcmp(buf, "one/two/three"));
	}

	SECTION("with memory allocation")
	{
		char *buf;

		REQUIRE(VACCEL_EINVAL ==
			path_from_parts(nullptr, "first", nullptr));
		REQUIRE(VACCEL_EINVAL == path_from_parts(&buf, nullptr));

		char one[PATH_MAX - 1];
		char two[PATH_MAX - 1];

		memset(one, 'a', sizeof(one) - 1);
		one[PATH_MAX - 2] = '\0';

		memset(two, 'b', sizeof(two) - 1);
		two[PATH_MAX - 2] = '\0';

		ret = path_from_parts(&buf, one, two, nullptr);
		REQUIRE(VACCEL_ENAMETOOLONG == ret);

		ret = path_from_parts(&buf, "one", "two", "three", nullptr);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(0 == strcmp(buf, "one/two/three"));
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
		REQUIRE(VACCEL_EINVAL ==
			path_file_name(nullptr, name, NAME_MAX, &alloc_name));
		REQUIRE(VACCEL_EINVAL ==
			path_file_name(nullptr, name, NAME_MAX, nullptr));
		REQUIRE(VACCEL_EINVAL == path_file_name(nullptr, nullptr,
							NAME_MAX, &alloc_name));
		REQUIRE(VACCEL_EINVAL ==
			path_file_name(nullptr, name, 0, &alloc_name));
		REQUIRE(VACCEL_EINVAL ==
			path_file_name(filepath, nullptr, NAME_MAX, nullptr));
		REQUIRE(VACCEL_EINVAL ==
			path_file_name(filepath, name, 0, nullptr));
		REQUIRE(VACCEL_EINVAL ==
			path_file_name(filepath, nullptr, 0, nullptr));
		REQUIRE(VACCEL_EINVAL ==
			path_file_name(nullptr, nullptr, 0, nullptr));
	}

	SECTION("name too long")
	{
		char name_toolong[NAME_MAX + 1];
		memset(name_toolong, 'a', NAME_MAX);
		name_toolong[NAME_MAX] = '\0';

		char *fullpath;
		ret = path_from_parts(&fullpath, "dir", name_toolong, nullptr);
		REQUIRE(VACCEL_OK == ret);

		REQUIRE(VACCEL_ENAMETOOLONG ==
			path_file_name(fullpath, nullptr, 0, &alloc_name));
		REQUIRE(alloc_name == nullptr);
		free(fullpath);
	}

	SECTION("size too small")
	{
		REQUIRE(VACCEL_EINVAL ==
			path_file_name(filepath, name, 1, nullptr));
	}

	SECTION("success")
	{
		REQUIRE(VACCEL_OK ==
			path_file_name(filepath, name, NAME_MAX, nullptr));
		REQUIRE(0 == strcmp(name, "file"));

		REQUIRE(VACCEL_OK ==
			path_file_name(filepath, nullptr, 0, &alloc_name));
		REQUIRE(0 == strcmp(name, "file"));
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
		REQUIRE(VACCEL_EINVAL == ret);

		ret = path_file_name_add_random_suffix(
			buf, 0, &ext_len, "path/to/file.txt", "suffix");
		REQUIRE(VACCEL_EINVAL == ret);

		ret = path_file_name_add_random_suffix(
			buf, PATH_MAX, nullptr, "path/to/file.txt", "suffix");
		REQUIRE(VACCEL_EINVAL == ret);

		ret = path_file_name_add_random_suffix(buf, PATH_MAX, &ext_len,
						       nullptr, "suffix");
		REQUIRE(VACCEL_EINVAL == ret);

		ret = path_file_name_add_random_suffix(
			buf, PATH_MAX, &ext_len, "path/to/file.txt", nullptr);
		REQUIRE(VACCEL_EINVAL == ret);
	}

	SECTION("OK with suffix")
	{
		ret = path_file_name_add_random_suffix(
			buf, PATH_MAX, &ext_len, "path/to/file.txt", "suffix");
		REQUIRE(VACCEL_OK == ret);
		REQUIRE(4 == ext_len);
		REQUIRE(0 == strcmp(buf, "path/to/filesuffix.txt"));
	}

	SECTION("Size too small")
	{
		ret = path_file_name_add_random_suffix(
			buf, 1, &ext_len, "path/to/file.txt", "suffix");
		REQUIRE(VACCEL_EINVAL == ret);
	}

	SECTION("No suffix")
	{
		memset(buf, 0, PATH_MAX);
		ret = path_file_name_add_random_suffix(
			buf, PATH_MAX, &ext_len, "path/to/file", "suffix");
		REQUIRE(VACCEL_OK == ret);
		REQUIRE(0 == ext_len);
		REQUIRE(0 == strcmp(buf, "path/to/filesuffix"));
	}
}
