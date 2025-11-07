// SPDX-License-Identifier: Apache-2.0

/*
 * The code below performs unit testing to resources.
 *
 * 1)  vaccel_blob_persist()
 * 2)  vaccel_blob_init()
 * 3)  vaccel_blob_init_from_buf()
 * 4)  vaccel_blob_release()
 * 5)  vaccel_blob_new()
 * 6)  vaccel_blob_from_buf()
 * 7)  vaccel_blob_delete()
 * 8)  vaccel_blob_initialized()
 * 9)  vaccel_blob_read()
 * 10)  vaccel_blob_data()
 * 11)  vaccel_blob_path()
 *
 */

#include "utils.hpp"
#include "vaccel.h"
#include <catch2/catch_test_macros.hpp>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <linux/limits.h>

TEST_CASE("blob_from_path", "[core][blob]")
{
	int ret;

	char path[PATH_MAX];
	ret = path_init_from_parts(path, PATH_MAX, BUILD_ROOT,
				   "examples/libmytestlib.so", nullptr);
	REQUIRE(ret == VACCEL_OK);

	struct vaccel_blob blob;
	struct vaccel_blob *alloc_blob;
	const size_t nr_blobs = 2;
	vaccel_blob *blobs[nr_blobs];

	ret = vaccel_blob_init(&blob, path);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(strcmp(blob.name, "libmytestlib.so") == 0);
	REQUIRE(strcmp(blob.path, path) == 0);
	REQUIRE(blob.path_owned == false);
	REQUIRE(blob.data == nullptr);
	REQUIRE(blob.size == 0);
	REQUIRE(blob.type == VACCEL_BLOB_FILE);
	REQUIRE_FALSE(blob.data_owned);
	blobs[0] = &blob;

	ret = vaccel_blob_new(&alloc_blob, path);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(strcmp(alloc_blob->name, "libmytestlib.so") == 0);
	REQUIRE(strcmp(alloc_blob->path, path) == 0);
	REQUIRE(alloc_blob->path_owned == false);
	REQUIRE(alloc_blob->data == nullptr);
	REQUIRE(alloc_blob->size == 0);
	REQUIRE(alloc_blob->type == VACCEL_BLOB_FILE);
	REQUIRE_FALSE(blob.data_owned);
	blobs[1] = alloc_blob;

	for (auto &blob : blobs) {
		unsigned char *buf;

		ret = vaccel_blob_read(blob);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(blob->data);
		REQUIRE(blob->size);
		REQUIRE(blob->type == VACCEL_BLOB_MAPPED);
		REQUIRE_FALSE(blob->data_owned);
		buf = blob->data;

		// file data not altered on re-read
		ret = vaccel_blob_read(blob);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(blob->data == buf);

		size_t len;
		ret = fs_file_read(path, (void **)&buf, &len);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(blob->size == len);
		for (size_t i = 0; i < len; i++)
			REQUIRE(blob->data[i] == buf[i]);
		free(buf);

		buf = vaccel_blob_data(blob, &len);
		REQUIRE(buf == blob->data);
		REQUIRE(len == blob->size);

		const char *fpath = vaccel_blob_path(blob);
		REQUIRE(fpath == blob->path);
	}

	ret = vaccel_blob_delete(alloc_blob);
	REQUIRE(ret == VACCEL_OK);

	ret = vaccel_blob_release(&blob);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(blob.name == nullptr);
	REQUIRE(blob.path == nullptr);
	REQUIRE(blob.path_owned == false);
	REQUIRE(blob.data == nullptr);
	REQUIRE(blob.size == 0);
	REQUIRE(blob.type == VACCEL_BLOB_MAX);
	REQUIRE_FALSE(blob.data_owned);
}

TEST_CASE("blob_from_buffer", "[core][blob]")
{
	int ret;
	char root_path[PATH_MAX];
	ret = path_init_from_parts(root_path, PATH_MAX, vaccel_rundir(), "test",
				   nullptr);
	REQUIRE(ret == VACCEL_OK);
	ret = fs_dir_create(root_path);
	REQUIRE(ret == VACCEL_OK);

	const char *file_name = "file";
	const char *alloc_file_name = "alloc_file";
	char file_path[PATH_MAX];
	char alloc_file_path[PATH_MAX];
	ret = path_init_from_parts(file_path, PATH_MAX, root_path, file_name,
				   nullptr);
	REQUIRE(ret == VACCEL_OK);
	ret = path_init_from_parts(alloc_file_path, PATH_MAX, root_path,
				   alloc_file_name, nullptr);
	REQUIRE(ret == VACCEL_OK);

	size_t len;
	unsigned char *buf;
	char path[PATH_MAX];
	ret = path_init_from_parts(path, PATH_MAX, BUILD_ROOT,
				   "examples/libmytestlib.so", nullptr);
	REQUIRE(ret == VACCEL_OK);
	ret = fs_file_read(path, (void **)&buf, &len);
	REQUIRE(ret == VACCEL_OK);

	struct vaccel_blob blob;
	struct vaccel_blob *alloc_blob;
	const size_t nr_blobs = 2;
	vaccel_blob *blobs[nr_blobs];

	ret = vaccel_blob_init_from_buf(&blob, buf, len, false, file_name,
					root_path, false);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(strcmp(blob.name, "file") == 0);
	REQUIRE(strcmp(blob.path, file_path) == 0);
	REQUIRE(blob.path_owned == true);
	REQUIRE(blob.data != nullptr);
	REQUIRE(blob.size == len);
	REQUIRE(blob.type == VACCEL_BLOB_MAPPED);
	REQUIRE(blob.data_owned);

	blobs[0] = &blob;

	SECTION("persist existing")
	{
		ret = vaccel_blob_from_buf(&alloc_blob, buf, len, false,
					   alloc_file_name, root_path, false);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(strstr(alloc_blob->name, alloc_file_name));
		REQUIRE(strstr(alloc_blob->path, alloc_file_path));
		REQUIRE(alloc_blob->path_owned == true);
		REQUIRE(alloc_blob->data != nullptr);
		REQUIRE(alloc_blob->size == len);
		REQUIRE(alloc_blob->type == VACCEL_BLOB_MAPPED);
		REQUIRE(alloc_blob->data_owned);

		for (size_t i = 0; i < len; i++)
			REQUIRE(alloc_blob->data[i] == buf[i]);

		ret = vaccel_blob_delete(alloc_blob);
		REQUIRE(ret == VACCEL_OK);
	}

	SECTION("persist random")
	{
		ret = vaccel_blob_from_buf(&alloc_blob, buf, len, false,
					   alloc_file_name, root_path, true);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(strstr(alloc_blob->name, alloc_file_name));
		REQUIRE(strstr(alloc_blob->path, alloc_file_path));
		REQUIRE(alloc_blob->path_owned == true);
		REQUIRE(alloc_blob->data != nullptr);
		REQUIRE(alloc_blob->size == len);
		REQUIRE(alloc_blob->type == VACCEL_BLOB_MAPPED);
		REQUIRE(alloc_blob->data_owned);

		for (size_t i = 0; i < len; i++)
			REQUIRE(alloc_blob->data[i] == buf[i]);

		ret = vaccel_blob_delete(alloc_blob);
		REQUIRE(ret == VACCEL_OK);
	}

	SECTION("no persist")
	{
		ret = vaccel_blob_from_buf(&alloc_blob, buf, len, false,
					   alloc_file_name, nullptr, false);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(strcmp(alloc_blob->name, alloc_file_name) == 0);
		REQUIRE(alloc_blob->path == nullptr);
		REQUIRE(alloc_blob->path_owned == false);
		REQUIRE(alloc_blob->data != nullptr);
		REQUIRE(alloc_blob->size == len);
		REQUIRE(alloc_blob->type == VACCEL_BLOB_BUFFER);
		REQUIRE_FALSE(alloc_blob->data_owned);

		for (size_t i = 0; i < len; i++)
			REQUIRE(alloc_blob->data[i] == buf[i]);

		ret = vaccel_blob_delete(alloc_blob);
		REQUIRE(ret == VACCEL_OK);
	}

	ret = vaccel_blob_from_buf(&alloc_blob, buf, len, false,
				   alloc_file_name, root_path, false);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(strcmp(alloc_blob->name, alloc_file_name) == 0);
	REQUIRE(strcmp(alloc_blob->path, alloc_file_path) == 0);
	REQUIRE(alloc_blob->path_owned == true);
	REQUIRE(alloc_blob->data != nullptr);
	REQUIRE(alloc_blob->size == len);
	REQUIRE(alloc_blob->type == VACCEL_BLOB_MAPPED);
	REQUIRE(alloc_blob->data_owned);

	blobs[1] = alloc_blob;

	for (auto &blob : blobs) {
		REQUIRE(blob->size == len);
		for (size_t i = 0; i < len; i++)
			REQUIRE(blob->data[i] == buf[i]);

		unsigned char *buf_ = vaccel_blob_data(blob, &len);
		REQUIRE(buf_ == blob->data);
		REQUIRE(len == blob->size);

		const char *fpath = vaccel_blob_path(blob);
		REQUIRE(fpath == blob->path);
	}

	ret = vaccel_blob_delete(alloc_blob);
	REQUIRE(ret == VACCEL_OK);

	ret = vaccel_blob_release(&blob);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(blob.name == nullptr);
	REQUIRE(blob.path == nullptr);
	REQUIRE(blob.path_owned == false);
	REQUIRE(blob.data == nullptr);
	REQUIRE(blob.size == 0);
	REQUIRE(blob.type == VACCEL_BLOB_MAX);
	REQUIRE_FALSE(blob.data_owned);

	ret = fs_dir_remove(root_path);
	REQUIRE(ret == VACCEL_OK);

	free(buf);
}

TEST_CASE("blob_persist_fail", "[core][blob]")
{
	int ret;
	struct vaccel_blob blob;
	const char *file_name = "file";
	char root_path[PATH_MAX];
	ret = path_init_from_parts(root_path, PATH_MAX, vaccel_rundir(), "test",
				   nullptr);
	REQUIRE(ret == VACCEL_OK);
	ret = fs_dir_create(root_path);
	REQUIRE(ret == VACCEL_OK);

	size_t len;
	unsigned char *buf;
	char path[PATH_MAX];
	ret = path_init_from_parts(path, PATH_MAX, BUILD_ROOT,
				   "examples/libmytestlib.so", nullptr);
	REQUIRE(ret == VACCEL_OK);
	ret = fs_file_read(path, (void **)&buf, &len);
	REQUIRE(ret == VACCEL_OK);

	SECTION("null arguments")
	{
		ret = vaccel_blob_init_from_buf(&blob, buf, len, false,
						file_name, nullptr, false);
		REQUIRE(ret == VACCEL_OK);

		ret = vaccel_blob_persist(&blob, root_path, nullptr, false);
		REQUIRE(ret == VACCEL_EINVAL);

		ret = vaccel_blob_persist(&blob, nullptr, file_name, false);
		REQUIRE(ret == VACCEL_ENOENT);

		ret = vaccel_blob_persist(nullptr, root_path, file_name, false);
		REQUIRE(ret == VACCEL_EINVAL);

		ret = vaccel_blob_persist(nullptr, nullptr, nullptr, false);
		REQUIRE(ret == VACCEL_EINVAL);

		ret = vaccel_blob_release(&blob);
		REQUIRE(ret == VACCEL_OK);
	}

	SECTION("existent file path")
	{
		ret = vaccel_blob_init_from_buf(&blob, buf, len, false,
						file_name, root_path, false);
		REQUIRE(ret == VACCEL_OK);

		ret = vaccel_blob_persist(&blob, root_path, file_name, false);
		REQUIRE(ret == VACCEL_EEXIST);

		ret = vaccel_blob_release(&blob);
		REQUIRE(ret == VACCEL_OK);
	}

	free(buf);

	ret = fs_dir_remove(root_path);
	REQUIRE(ret == VACCEL_OK);
}

TEST_CASE("blob_init_fail", "[core][blob]")
{
	int ret;
	char root_path[PATH_MAX];
	ret = path_init_from_parts(root_path, PATH_MAX, vaccel_rundir(), "test",
				   nullptr);
	REQUIRE(ret == VACCEL_OK);
	ret = fs_dir_create(root_path);
	REQUIRE(ret == VACCEL_OK);

	size_t len;
	unsigned char *buf;
	char path[PATH_MAX];
	ret = path_init_from_parts(path, PATH_MAX, BUILD_ROOT,
				   "examples/libmytestlib.so", nullptr);
	REQUIRE(ret == VACCEL_OK);
	ret = fs_file_read(path, (void **)&buf, &len);
	REQUIRE(ret == VACCEL_OK);

	const char *file_name = "file";
	struct vaccel_blob blob;
	struct vaccel_blob *alloc_blob;

	SECTION("null arguments")
	{
		ret = vaccel_blob_init(&blob, nullptr);
		REQUIRE(ret == VACCEL_EINVAL);

		ret = vaccel_blob_init(nullptr, path);
		REQUIRE(ret == VACCEL_EINVAL);

		ret = vaccel_blob_init_from_buf(&blob, nullptr, len, false,
						file_name, root_path, false);
		REQUIRE(ret == VACCEL_EINVAL);

		ret = vaccel_blob_init_from_buf(&blob, buf, 0, false, file_name,
						root_path, false);
		REQUIRE(ret == VACCEL_EINVAL);

		ret = vaccel_blob_init_from_buf(&blob, buf, len, false, nullptr,
						root_path, false);
		REQUIRE(ret == VACCEL_EINVAL);

		ret = vaccel_blob_new(&alloc_blob, nullptr);
		REQUIRE(ret == VACCEL_EINVAL);

		ret = vaccel_blob_new(nullptr, path);
		REQUIRE(ret == VACCEL_EINVAL);

		ret = vaccel_blob_from_buf(&alloc_blob, nullptr, len, false,
					   file_name, root_path, false);
		REQUIRE(ret == VACCEL_EINVAL);

		ret = vaccel_blob_from_buf(&alloc_blob, buf, 0, false,
					   file_name, root_path, false);
		REQUIRE(ret == VACCEL_EINVAL);

		ret = vaccel_blob_from_buf(&alloc_blob, buf, len, false,
					   nullptr, root_path, false);
		REQUIRE(ret == VACCEL_EINVAL);
	}

	SECTION("non existent file or dir path")
	{
		const char *non_existent_path = "non_existent_path";

		ret = vaccel_blob_init(&blob, non_existent_path);
		REQUIRE(ret == VACCEL_EINVAL);

		ret = vaccel_blob_new(&alloc_blob, non_existent_path);
		REQUIRE(ret == VACCEL_EINVAL);

		ret = vaccel_blob_init_from_buf(&blob, buf, len, false,
						file_name, non_existent_path,
						false);
		REQUIRE(ret == VACCEL_ENOENT);

		ret = vaccel_blob_from_buf(&alloc_blob, buf, len, false,
					   file_name, non_existent_path, false);
		REQUIRE(ret == VACCEL_ENOENT);
	}

	ret = fs_dir_remove(root_path);
	REQUIRE(ret == VACCEL_OK);

	free(buf);
}

TEST_CASE("blob_release_fail", "[core][blob]")
{
	int ret;
	struct vaccel_blob blob;

	SECTION("null arguments")
	{
		ret = vaccel_blob_release(nullptr);
		REQUIRE(ret == VACCEL_EINVAL);
	}

	SECTION("uninitialized blob")
	{
		blob.name = nullptr;
		blob.path = nullptr;
		blob.data = nullptr;
		blob.size = 0;
		blob.type = VACCEL_BLOB_MAX;
		ret = vaccel_blob_release(&blob);
		REQUIRE(ret == VACCEL_EINVAL);
	}
}

TEST_CASE("blob_read_fail", "[core][blob]")
{
	int ret;
	struct vaccel_blob blob;

	SECTION("null arguments")
	{
		ret = vaccel_blob_read(nullptr);
		REQUIRE(ret == VACCEL_EINVAL);
	}

	SECTION("uninitialized blob")
	{
		blob.name = nullptr;
		blob.path = nullptr;
		blob.data = nullptr;
		blob.size = 0;
		blob.type = VACCEL_BLOB_MAX;
		ret = vaccel_blob_read(&blob);
		REQUIRE(ret == VACCEL_EINVAL);
	}
}

TEST_CASE("blob_info_fail", "[core][blob]")
{
	unsigned char *data = vaccel_blob_data(nullptr, nullptr);
	REQUIRE(data == nullptr);

	const char *path = vaccel_blob_path(nullptr);
	REQUIRE(path == nullptr);
}

TEST_CASE("blob_from_buffer_owned_data", "[core][blob]")
{
	int ret;
	char root_path[PATH_MAX];
	ret = path_init_from_parts(root_path, PATH_MAX, vaccel_rundir(), "test",
				   nullptr);
	REQUIRE(ret == VACCEL_OK);
	ret = fs_dir_create(root_path);
	REQUIRE(ret == VACCEL_OK);

	const char *file_name = "file";
	const char *alloc_file_name = "alloc_file";
	char file_path[PATH_MAX];
	char alloc_file_path[PATH_MAX];
	ret = path_init_from_parts(file_path, PATH_MAX, root_path, file_name,
				   nullptr);
	REQUIRE(ret == VACCEL_OK);
	ret = path_init_from_parts(alloc_file_path, PATH_MAX, root_path,
				   alloc_file_name, nullptr);
	REQUIRE(ret == VACCEL_OK);

	size_t len;
	unsigned char *buf;
	char path[PATH_MAX];
	ret = path_init_from_parts(path, PATH_MAX, BUILD_ROOT,
				   "examples/libmytestlib.so", nullptr);
	REQUIRE(ret == VACCEL_OK);
	ret = fs_file_read(path, (void **)&buf, &len);
	REQUIRE(ret == VACCEL_OK);

	struct vaccel_blob blob;
	struct vaccel_blob *alloc_blob;
	const size_t nr_blobs = 2;
	vaccel_blob *blobs[nr_blobs];

	ret = vaccel_blob_init_from_buf(&blob, buf, len, true, file_name,
					root_path, false);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(strcmp(blob.name, "file") == 0);
	REQUIRE(strcmp(blob.path, file_path) == 0);
	REQUIRE(blob.path_owned == true);
	REQUIRE(blob.data != nullptr);
	REQUIRE(blob.data != buf);
	REQUIRE(blob.size == len);
	REQUIRE(blob.type == VACCEL_BLOB_MAPPED);
	REQUIRE(blob.data_owned);

	blobs[0] = &blob;

	SECTION("persist existing")
	{
		ret = vaccel_blob_from_buf(&alloc_blob, buf, len, true,
					   alloc_file_name, root_path, false);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(strstr(alloc_blob->name, alloc_file_name));
		REQUIRE(strstr(alloc_blob->path, alloc_file_path));
		REQUIRE(alloc_blob->path_owned == true);
		REQUIRE(alloc_blob->data != nullptr);
		REQUIRE(alloc_blob->data != buf);
		REQUIRE(alloc_blob->size == len);
		REQUIRE(alloc_blob->type == VACCEL_BLOB_MAPPED);
		REQUIRE(alloc_blob->data_owned);

		for (size_t i = 0; i < len; i++)
			REQUIRE(alloc_blob->data[i] == buf[i]);

		ret = vaccel_blob_delete(alloc_blob);
		REQUIRE(ret == VACCEL_OK);
	}

	SECTION("persist random")
	{
		ret = vaccel_blob_from_buf(&alloc_blob, buf, len, true,
					   alloc_file_name, root_path, true);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(strstr(alloc_blob->name, alloc_file_name));
		REQUIRE(strstr(alloc_blob->path, alloc_file_path));
		REQUIRE(alloc_blob->path_owned == true);
		REQUIRE(alloc_blob->data != nullptr);
		REQUIRE(alloc_blob->data != buf);
		REQUIRE(alloc_blob->size == len);
		REQUIRE(alloc_blob->type == VACCEL_BLOB_MAPPED);
		REQUIRE(alloc_blob->data_owned);

		for (size_t i = 0; i < len; i++)
			REQUIRE(alloc_blob->data[i] == buf[i]);

		ret = vaccel_blob_delete(alloc_blob);
		REQUIRE(ret == VACCEL_OK);
	}

	SECTION("persist later")
	{
		ret = vaccel_blob_from_buf(&alloc_blob, buf, len, true,
					   alloc_file_name, nullptr, false);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(strstr(alloc_blob->name, alloc_file_name));
		REQUIRE(alloc_blob->path_owned == false);
		REQUIRE(alloc_blob->data != nullptr);
		REQUIRE(alloc_blob->data != buf);
		REQUIRE(alloc_blob->size == len);
		REQUIRE(alloc_blob->type == VACCEL_BLOB_BUFFER);
		REQUIRE(alloc_blob->data_owned);

		for (size_t i = 0; i < len; i++)
			REQUIRE(alloc_blob->data[i] == buf[i]);

		ret = vaccel_blob_persist(alloc_blob, root_path,
					  alloc_file_name, false);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(alloc_blob->type == VACCEL_BLOB_MAPPED);
		REQUIRE(alloc_blob->data_owned);

		ret = vaccel_blob_delete(alloc_blob);
		REQUIRE(ret == VACCEL_OK);
	}

	SECTION("no persist")
	{
		ret = vaccel_blob_from_buf(&alloc_blob, buf, len, true,
					   alloc_file_name, nullptr, false);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(strcmp(alloc_blob->name, alloc_file_name) == 0);
		REQUIRE(alloc_blob->path == nullptr);
		REQUIRE(alloc_blob->path_owned == false);
		REQUIRE(alloc_blob->data != nullptr);
		REQUIRE(alloc_blob->data != buf);
		REQUIRE(alloc_blob->size == len);
		REQUIRE(alloc_blob->type == VACCEL_BLOB_BUFFER);
		REQUIRE(alloc_blob->data_owned);

		for (size_t i = 0; i < len; i++)
			REQUIRE(alloc_blob->data[i] == buf[i]);

		ret = vaccel_blob_delete(alloc_blob);
		REQUIRE(ret == VACCEL_OK);
	}

	ret = vaccel_blob_from_buf(&alloc_blob, buf, len, true, alloc_file_name,
				   root_path, false);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(strcmp(alloc_blob->name, alloc_file_name) == 0);
	REQUIRE(strcmp(alloc_blob->path, alloc_file_path) == 0);
	REQUIRE(alloc_blob->path_owned == true);
	REQUIRE(alloc_blob->data != nullptr);
	REQUIRE(alloc_blob->data != buf);
	REQUIRE(alloc_blob->size == len);
	REQUIRE(alloc_blob->type == VACCEL_BLOB_MAPPED);
	REQUIRE(alloc_blob->data_owned);

	blobs[1] = alloc_blob;

	for (auto &blob : blobs) {
		REQUIRE(blob->size == len);
		for (size_t i = 0; i < len; i++)
			REQUIRE(blob->data[i] == buf[i]);

		unsigned char *buf_ = vaccel_blob_data(blob, &len);
		REQUIRE(buf_ == blob->data);
		REQUIRE(len == blob->size);

		const char *fpath = vaccel_blob_path(blob);
		REQUIRE(fpath == blob->path);
	}

	ret = vaccel_blob_delete(alloc_blob);
	REQUIRE(ret == VACCEL_OK);

	ret = vaccel_blob_release(&blob);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(blob.name == nullptr);
	REQUIRE(blob.path == nullptr);
	REQUIRE(blob.path_owned == false);
	REQUIRE(blob.data == nullptr);
	REQUIRE(blob.size == 0);
	REQUIRE(blob.type == VACCEL_BLOB_MAX);
	REQUIRE_FALSE(blob.data_owned);

	ret = fs_dir_remove(root_path);
	REQUIRE(ret == VACCEL_OK);

	free(buf);
}
