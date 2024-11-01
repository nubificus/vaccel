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

TEST_CASE("path_is_url", "[utils/path]")
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

TEST_CASE("path_from_parts", "[utils/path]")
{
	int ret;

	SECTION("init")
	{
		char buf[64] = { 0 };

		REQUIRE(VACCEL_EINVAL ==
			path_init_from_parts(nullptr, 0, nullptr));

		ret = path_init_from_parts(buf, PATH_MAX + 1, "one", "two",
					   NULL);
		REQUIRE(VACCEL_ENAMETOOLONG == ret);

		ret = path_init_from_parts(buf, 10, "longer_than_10", "two",
					   "three", NULL);
		REQUIRE(VACCEL_ENAMETOOLONG == ret);

		ret = path_init_from_parts(buf, 5, "one", "two", "three", NULL);
		REQUIRE(VACCEL_ENAMETOOLONG == ret);

		ret = path_init_from_parts(buf, sizeof(buf), "one", "two",
					   "three", NULL);
		REQUIRE(0 == ret);
		REQUIRE(0 == strcmp(buf, "one/two/three"));
	}

	SECTION("with memory allocation")
	{
		char *buf;

		REQUIRE(VACCEL_EINVAL ==
			path_from_parts(nullptr, "first", NULL));
		REQUIRE(VACCEL_EINVAL == path_from_parts(&buf, nullptr));

		char one[PATH_MAX - 1];
		char two[PATH_MAX - 1];

		memset(one, 'a', sizeof(one) - 1);
		one[PATH_MAX - 2] = '\0';

		memset(two, 'b', sizeof(two) - 1);
		two[PATH_MAX - 2] = '\0';

		ret = path_from_parts(&buf, one, two, NULL);
		REQUIRE(VACCEL_ENAMETOOLONG == ret);

		ret = path_from_parts(&buf, "one", "two", "three", NULL);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(0 == strcmp(buf, "one/two/three"));
		free(buf);
	}
}

TEST_CASE("path_file_name", "[utils/path]")
{
	char *buf;
	int ret;

	REQUIRE(VACCEL_EINVAL == path_file_name(&buf, nullptr));
	REQUIRE(VACCEL_EINVAL == path_file_name(nullptr, "path"));

	char name[NAME_MAX + 1];
	memset(name, 'a', NAME_MAX);
	name[NAME_MAX] = '\0';

	char *fullpath;
	ret = path_from_parts(&fullpath, "dir", name, NULL);
	REQUIRE(VACCEL_OK == ret);
	REQUIRE(VACCEL_ENAMETOOLONG == path_file_name(&buf, fullpath));

	REQUIRE(VACCEL_OK == path_file_name(&buf, "/path/to/file"));
	REQUIRE(0 == strcmp(buf, "file"));

	free(fullpath);
	free(buf);
}

TEST_CASE("path_file_name_add_random_suffix", "[utils/path]")
{
	char buf[128] = { 0 };
	size_t ext_len;
	int ret;

	SECTION("NULL Arguments")
	{
		ret = path_file_name_add_random_suffix(
			nullptr, &ext_len, "path/to/file.txt", "suffix");
		REQUIRE(VACCEL_EINVAL == ret);

		ret = path_file_name_add_random_suffix(
			buf, nullptr, "path/to/file.txt", "suffix");
		REQUIRE(VACCEL_EINVAL == ret);

		ret = path_file_name_add_random_suffix(buf, &ext_len, nullptr,
						       "suffix");
		REQUIRE(VACCEL_EINVAL == ret);

		ret = path_file_name_add_random_suffix(
			buf, &ext_len, "path/to/file.txt", nullptr);
		REQUIRE(VACCEL_EINVAL == ret);
	}

	SECTION("OK with suffix")
	{
		ret = path_file_name_add_random_suffix(
			buf, &ext_len, "path/to/file.txt", "suffix");
		REQUIRE(VACCEL_OK == ret);
		REQUIRE(4 == ext_len);
		REQUIRE(0 == strcmp(buf, "path/to/filesuffix.txt"));
	}

	SECTION("No suffix")
	{
		memset(buf, 0, 128);
		ret = path_file_name_add_random_suffix(
			buf, &ext_len, "path/to/file", "suffix");
		REQUIRE(VACCEL_OK == ret);
		REQUIRE(0 == ext_len);
		REQUIRE(0 == strcmp(buf, "path/to/filesuffix"));
	}
}
