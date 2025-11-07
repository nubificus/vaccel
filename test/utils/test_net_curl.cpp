// SPDX-License-Identifier: Apache-2.0

/*
 * The code below performs unit testing to `net` functions (curl variants).
 *
 * 1) net_path_is_url()
 * 2) net_path_exists()
 * 3) net_file_download()
 *
 */

#include "utils.hpp"
#include "vaccel.h"
#include <catch2/catch_test_macros.hpp>
#include <fff.h>
#include <linux/limits.h>

DEFINE_FFF_GLOBALS;

extern "C" {
FAKE_VALUE_FUNC(bool, net_nocurl_path_is_url, const char *);
FAKE_VALUE_FUNC(bool, net_nocurl_path_exists, const char *);
FAKE_VALUE_FUNC(int, net_nocurl_file_download, const char *, const char *);
}

TEST_CASE("net_path_is_url", "[utils][net][curl]")
{
	RESET_FAKE(net_nocurl_path_is_url);
	RESET_FAKE(net_nocurl_path_exists);
	RESET_FAKE(net_nocurl_file_download);

	const char *url = "http://nubificus.co.uk";
	const char *word = "hello world";

	SECTION("is url")
	{
		net_nocurl_path_is_url_fake.return_val = true;
		REQUIRE(net_path_is_url(url));
#ifdef USE_LIBCURL
		REQUIRE(net_nocurl_path_is_url_fake.call_count == 0);
#else
		// emulate net_curl_path_is_url()
		REQUIRE(net_nocurl_path_is_url_fake.call_count == 1);
#endif
	}

	RESET_FAKE(net_nocurl_path_is_url);

	SECTION("is not url")
	{
		net_nocurl_path_is_url_fake.return_val = false;
		REQUIRE_FALSE(net_path_is_url(word));
#ifdef USE_LIBCURL
		REQUIRE(net_nocurl_path_is_url_fake.call_count == 0);
#else
		// emulate net_curl_path_is_url()
		REQUIRE(net_nocurl_path_is_url_fake.call_count == 1);
#endif
	}

	RESET_FAKE(net_nocurl_path_is_url);

	SECTION("null arguments")
	{
		REQUIRE_FALSE(net_path_is_url(nullptr));
		REQUIRE(net_nocurl_path_is_url_fake.call_count == 0);
	}
}

TEST_CASE("net_path_exists", "[utils][net][curl]")
{
	RESET_FAKE(net_nocurl_path_is_url);
	RESET_FAKE(net_nocurl_path_exists);
	RESET_FAKE(net_nocurl_file_download);

	const char *existent_url = "http://nubificus.co.uk";
	const char *non_existent_url = "http://nubificus.co.uk.not";

	SECTION("url exists")
	{
		net_nocurl_path_exists_fake.return_val = true;
		REQUIRE(net_path_exists(existent_url));
#ifdef USE_LIBCURL
		REQUIRE(net_nocurl_path_exists_fake.call_count == 0);
#else
		// emulate net_curl_path_exists()
		REQUIRE(net_nocurl_path_exists_fake.call_count == 1);
#endif
	}

	RESET_FAKE(net_nocurl_path_exists);

	SECTION("url doesn't exist")
	{
		net_nocurl_path_exists_fake.return_val = false;
		REQUIRE_FALSE(net_path_exists(non_existent_url));
#ifdef USE_LIBCURL
		REQUIRE(net_nocurl_path_exists_fake.call_count == 0);
#else
		// emulate net_curl_path_exists()
		REQUIRE(net_nocurl_path_exists_fake.call_count == 1);
#endif
	}

	RESET_FAKE(net_nocurl_path_exists);

	SECTION("null arguments")
	{
		REQUIRE_FALSE(net_path_exists(nullptr));
		REQUIRE(net_nocurl_path_exists_fake.call_count == 0);
	}
}

TEST_CASE("net_file_download", "[utils][net][curl]")
{
	RESET_FAKE(net_nocurl_path_is_url);
	RESET_FAKE(net_nocurl_path_exists);
	RESET_FAKE(net_nocurl_file_download);

	char existent_url[PATH_MAX];
	char non_existent_url[PATH_MAX];
	char root_path[PATH_MAX];
	char download_path[PATH_MAX];

	REQUIRE(path_init_from_parts(existent_url, PATH_MAX, REPO_RAWURL,
				     "examples/models/torch/cnn_trace.pt",
				     nullptr) == VACCEL_OK);
	REQUIRE(path_init_from_parts(non_existent_url, PATH_MAX, REPO_RAWURL,
				     "examples/models/torch/cnn_trace.pt.not",
				     nullptr) == VACCEL_OK);
	REQUIRE(path_init_from_parts(root_path, PATH_MAX, vaccel_rundir(),
				     "test", nullptr) == VACCEL_OK);
	REQUIRE(path_init_from_parts(download_path, PATH_MAX, root_path,
				     "cnn_trace.pt", nullptr) == VACCEL_OK);

	int const ret = fs_dir_create(root_path);
	REQUIRE((ret == VACCEL_OK || ret == VACCEL_EEXIST));

	RESET_FAKE(net_nocurl_file_download);

	SECTION("url exists")
	{
		net_nocurl_file_download_fake.return_val = VACCEL_OK;
		REQUIRE(net_file_download(existent_url, download_path) ==
			VACCEL_OK);
#ifdef USE_LIBCURL
		REQUIRE(fs_path_is_file(download_path));
		REQUIRE(fs_file_remove(download_path) == VACCEL_OK);
		REQUIRE(net_nocurl_file_download_fake.call_count == 0);
#else
		// emulate net_curl_file_download()
		REQUIRE(!fs_path_is_file(download_path));
		REQUIRE(net_nocurl_file_download_fake.call_count == 1);
#endif
	}

	RESET_FAKE(net_nocurl_file_download);

	SECTION("url doesn't exist")
	{
		net_nocurl_file_download_fake.return_val = VACCEL_EREMOTEIO;
		REQUIRE(net_file_download(non_existent_url, download_path) ==
			VACCEL_EREMOTEIO);
#ifdef USE_LIBCURL
		REQUIRE(fs_path_is_file(download_path));
		REQUIRE(fs_file_remove(download_path) == VACCEL_OK);
		REQUIRE(net_nocurl_file_download_fake.call_count == 0);
#else
		// emulate net_curl_file_download()
		REQUIRE(!fs_path_is_file(download_path));
		REQUIRE(net_nocurl_file_download_fake.call_count == 1);
#endif
	}

	RESET_FAKE(net_nocurl_file_download);

	SECTION("local file already exists")
	{
		REQUIRE(fs_file_create(download_path, nullptr) == VACCEL_OK);
		REQUIRE(fs_path_is_file(download_path));

		REQUIRE(net_file_download(existent_url, download_path) ==
			VACCEL_EEXIST);

		REQUIRE(fs_file_remove(download_path) == VACCEL_OK);

		REQUIRE(net_nocurl_file_download_fake.call_count == 0);
	}

	RESET_FAKE(net_nocurl_file_download);

	SECTION("null arguments")
	{
		REQUIRE(net_file_download(nullptr, download_path) ==
			VACCEL_EINVAL);
		REQUIRE(net_file_download(existent_url, nullptr) ==
			VACCEL_EINVAL);
		REQUIRE(net_file_download(nullptr, nullptr) == VACCEL_EINVAL);
		REQUIRE(net_nocurl_file_download_fake.call_count == 0);
	}

	REQUIRE(fs_dir_remove(root_path) == VACCEL_OK);
}
