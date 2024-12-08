// SPDX-License-Identifier: Apache-2.0

/*
 * The code below performs unit testing to resources.
 *
 * 1)  vaccel_file_persist()
 * 2)  vaccel_file_init()
 * 3)  vaccel_file_init_from_buf()
 * 4)  vaccel_file_release()
 * 5)  vaccel_file_new()
 * 6)  vaccel_file_from_buf()
 * 7)  vaccel_file_delete()
 * 8)  vaccel_file_initialized()
 * 9)  vaccel_file_read()
 * 10)  vaccel_file_data()
 * 11)  vaccel_file_path()
 *
 */

#include "utils.hpp"
#include "vaccel.h"
#include <catch.hpp>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <linux/limits.h>

TEST_CASE("file_from_path", "[core][file]")
{
	int ret;

	char path[PATH_MAX];
	ret = path_init_from_parts(path, PATH_MAX, BUILD_ROOT,
				   "examples/libmytestlib.so", nullptr);
	REQUIRE(ret == VACCEL_OK);

	struct vaccel_file file;
	struct vaccel_file *alloc_file;
	const size_t nr_files = 2;
	vaccel_file *files[nr_files];

	ret = vaccel_file_init(&file, path);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(strcmp(file.name, "libmytestlib.so") == 0);
	REQUIRE(strcmp(file.path, path) == 0);
	REQUIRE(file.path_owned == false);
	REQUIRE(file.data == nullptr);
	REQUIRE(file.size == 0);
	files[0] = &file;

	ret = vaccel_file_new(&alloc_file, path);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(strcmp(alloc_file->name, "libmytestlib.so") == 0);
	REQUIRE(strcmp(alloc_file->path, path) == 0);
	REQUIRE(alloc_file->path_owned == false);
	REQUIRE(alloc_file->data == nullptr);
	REQUIRE(alloc_file->size == 0);
	files[1] = alloc_file;

	for (auto &file : files) {
		unsigned char *buf;

		ret = vaccel_file_read(file);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(file->data);
		REQUIRE(file->size);
		buf = file->data;

		// file data not altered on re-read
		ret = vaccel_file_read(file);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(file->data == buf);

		size_t len;
		ret = fs_file_read(path, (void **)&buf, &len);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(file->size == len);
		for (size_t i = 0; i < len; i++)
			REQUIRE(file->data[i] == buf[i]);
		free(buf);

		buf = vaccel_file_data(file, &len);
		REQUIRE(buf == file->data);
		REQUIRE(len == file->size);

		const char *fpath = vaccel_file_path(file);
		REQUIRE(fpath == file->path);
	}

	ret = vaccel_file_delete(alloc_file);
	REQUIRE(ret == VACCEL_OK);

	ret = vaccel_file_release(&file);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(file.name == nullptr);
	REQUIRE(file.path == nullptr);
	REQUIRE(file.path_owned == false);
	REQUIRE(file.data == nullptr);
	REQUIRE(file.size == 0);
}

TEST_CASE("file_from_buffer", "[core][file]")
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

	struct vaccel_file file;
	struct vaccel_file *alloc_file;
	const size_t nr_files = 2;
	vaccel_file *files[nr_files];

	ret = vaccel_file_init_from_buf(&file, buf, len, file_name, root_path,
					false);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(strcmp(file.name, "file") == 0);
	REQUIRE(strcmp(file.path, file_path) == 0);
	REQUIRE(file.path_owned == true);
	REQUIRE(file.data != nullptr);
	REQUIRE(file.size);
	files[0] = &file;

	SECTION("persist existing")
	{
		ret = vaccel_file_from_buf(&alloc_file, buf, len,
					   alloc_file_name, root_path, false);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(strstr(alloc_file->name, alloc_file_name));
		REQUIRE(strstr(alloc_file->path, alloc_file_path));
		REQUIRE(alloc_file->path_owned == true);
		REQUIRE(alloc_file->data != nullptr);
		REQUIRE(alloc_file->size == len);

		for (size_t i = 0; i < len; i++)
			REQUIRE(alloc_file->data[i] == buf[i]);

		ret = vaccel_file_delete(alloc_file);
		REQUIRE(ret == VACCEL_OK);
	}

	SECTION("persist random")
	{
		ret = vaccel_file_from_buf(&alloc_file, buf, len,
					   alloc_file_name, root_path, true);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(strstr(alloc_file->name, alloc_file_name));
		REQUIRE(strstr(alloc_file->path, alloc_file_path));
		REQUIRE(alloc_file->path_owned == true);
		REQUIRE(alloc_file->data != nullptr);
		REQUIRE(alloc_file->size == len);

		for (size_t i = 0; i < len; i++)
			REQUIRE(alloc_file->data[i] == buf[i]);

		ret = vaccel_file_delete(alloc_file);
		REQUIRE(ret == VACCEL_OK);
	}

	SECTION("no persist")
	{
		ret = vaccel_file_from_buf(&alloc_file, buf, len,
					   alloc_file_name, nullptr, false);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(strcmp(alloc_file->name, alloc_file_name) == 0);
		REQUIRE(alloc_file->path == nullptr);
		REQUIRE(alloc_file->path_owned == false);
		REQUIRE(alloc_file->data != nullptr);
		REQUIRE(alloc_file->size == len);

		for (size_t i = 0; i < len; i++)
			REQUIRE(alloc_file->data[i] == buf[i]);

		ret = vaccel_file_delete(alloc_file);
		REQUIRE(ret == VACCEL_OK);
	}

	ret = vaccel_file_from_buf(&alloc_file, buf, len, alloc_file_name,
				   root_path, false);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(strcmp(alloc_file->name, alloc_file_name) == 0);
	REQUIRE(strcmp(alloc_file->path, alloc_file_path) == 0);
	REQUIRE(alloc_file->path_owned == true);
	REQUIRE(alloc_file->data != nullptr);
	REQUIRE(alloc_file->size);
	files[1] = alloc_file;

	for (auto &file : files) {
		REQUIRE(file->size == len);
		for (size_t i = 0; i < len; i++)
			REQUIRE(file->data[i] == buf[i]);

		unsigned char *buf_ = vaccel_file_data(file, &len);
		REQUIRE(buf_ == file->data);
		REQUIRE(len == file->size);

		const char *fpath = vaccel_file_path(file);
		REQUIRE(fpath == file->path);
	}

	ret = vaccel_file_delete(alloc_file);
	REQUIRE(ret == VACCEL_OK);

	ret = vaccel_file_release(&file);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(file.name == nullptr);
	REQUIRE(file.path == nullptr);
	REQUIRE(file.path_owned == false);
	REQUIRE(file.data == nullptr);
	REQUIRE(file.size == 0);

	ret = fs_dir_remove(root_path);
	REQUIRE(ret == VACCEL_OK);

	free(buf);
}

TEST_CASE("file_persist_fail", "[core][file]")
{
	int ret;
	struct vaccel_file file;
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
		ret = vaccel_file_init_from_buf(&file, buf, len, file_name,
						nullptr, false);
		REQUIRE(ret == VACCEL_OK);

		ret = vaccel_file_persist(&file, root_path, nullptr, false);
		REQUIRE(ret == VACCEL_EINVAL);

		ret = vaccel_file_persist(&file, nullptr, file_name, false);
		REQUIRE(ret == VACCEL_ENOENT);

		ret = vaccel_file_persist(nullptr, root_path, file_name, false);
		REQUIRE(ret == VACCEL_EINVAL);

		ret = vaccel_file_persist(nullptr, nullptr, nullptr, false);
		REQUIRE(ret == VACCEL_EINVAL);

		ret = vaccel_file_release(&file);
		REQUIRE(ret == VACCEL_OK);
	}

	SECTION("existent file path")
	{
		ret = vaccel_file_init_from_buf(&file, buf, len, file_name,
						root_path, false);
		REQUIRE(ret == VACCEL_OK);

		ret = vaccel_file_persist(&file, root_path, file_name, false);
		REQUIRE(ret == VACCEL_EEXIST);

		ret = vaccel_file_release(&file);
		REQUIRE(ret == VACCEL_OK);
	}

	free(buf);

	ret = fs_dir_remove(root_path);
	REQUIRE(ret == VACCEL_OK);
}

TEST_CASE("file_init_fail", "[core][file]")
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
	struct vaccel_file file;
	struct vaccel_file *alloc_file;

	SECTION("null arguments")
	{
		ret = vaccel_file_init(&file, nullptr);
		REQUIRE(ret == VACCEL_EINVAL);

		ret = vaccel_file_init(nullptr, path);
		REQUIRE(ret == VACCEL_EINVAL);

		ret = vaccel_file_init_from_buf(&file, nullptr, len, file_name,
						root_path, false);
		REQUIRE(ret == VACCEL_EINVAL);

		ret = vaccel_file_init_from_buf(&file, buf, 0, file_name,
						root_path, false);
		REQUIRE(ret == VACCEL_EINVAL);

		ret = vaccel_file_init_from_buf(&file, buf, len, nullptr,
						root_path, false);
		REQUIRE(ret == VACCEL_EINVAL);

		ret = vaccel_file_new(&alloc_file, nullptr);
		REQUIRE(ret == VACCEL_EINVAL);

		ret = vaccel_file_new(nullptr, path);
		REQUIRE(ret == VACCEL_EINVAL);

		ret = vaccel_file_from_buf(&alloc_file, nullptr, len, file_name,
					   root_path, false);
		REQUIRE(ret == VACCEL_EINVAL);

		ret = vaccel_file_from_buf(&alloc_file, buf, 0, file_name,
					   root_path, false);
		REQUIRE(ret == VACCEL_EINVAL);

		ret = vaccel_file_from_buf(&alloc_file, buf, len, nullptr,
					   root_path, false);
		REQUIRE(ret == VACCEL_EINVAL);
	}

	SECTION("non existent file or dir path")
	{
		const char *non_existent_path = "non_existent_path";

		ret = vaccel_file_init(&file, non_existent_path);
		REQUIRE(ret == VACCEL_EINVAL);

		ret = vaccel_file_new(&alloc_file, non_existent_path);
		REQUIRE(ret == VACCEL_EINVAL);

		ret = vaccel_file_init_from_buf(&file, buf, len, file_name,
						non_existent_path, false);
		REQUIRE(ret == VACCEL_ENOENT);

		ret = vaccel_file_from_buf(&alloc_file, buf, len, file_name,
					   non_existent_path, false);
		REQUIRE(ret == VACCEL_ENOENT);
	}

	ret = fs_dir_remove(root_path);
	REQUIRE(ret == VACCEL_OK);

	free(buf);
}

TEST_CASE("file_release_fail", "[core][file]")
{
	int ret;
	struct vaccel_file file;

	SECTION("null arguments")
	{
		ret = vaccel_file_release(nullptr);
		REQUIRE(ret == VACCEL_EINVAL);
	}

	SECTION("uninitialized file")
	{
		file.name = nullptr;
		file.path = nullptr;
		file.data = nullptr;
		file.size = 0;
		ret = vaccel_file_release(&file);
		REQUIRE(ret == VACCEL_EINVAL);
	}
}

TEST_CASE("file_read_fail", "[core][file]")
{
	int ret;
	struct vaccel_file file;

	SECTION("null arguments")
	{
		ret = vaccel_file_read(nullptr);
		REQUIRE(ret == VACCEL_EINVAL);
	}

	SECTION("uninitialized file")
	{
		file.name = nullptr;
		file.path = nullptr;
		file.data = nullptr;
		file.size = 0;
		ret = vaccel_file_read(&file);
		REQUIRE(ret == VACCEL_EINVAL);
	}
}

TEST_CASE("file_info_fail", "[core][file]")
{
	unsigned char *data = vaccel_file_data(nullptr, nullptr);
	REQUIRE(data == nullptr);

	const char *path = vaccel_file_path(nullptr);
	REQUIRE(path == nullptr);
}
