// SPDX-License-Identifier: Apache-2.0

/*
 *
 * The code below performs unit testing to resources.
 *
 * 1)  resource_bootstrap()
 * 2)  resource_new()
 * 3)  resource_destroy()
 * 4)  resource_create_rundir()
 * 5)  vaccel_resource_get_by_id()
 * 6)  vaccel_resource_init(), from dir
 * 7)  vaccel_resource_init(), from file
 * 8)  vaccel_resource_init(), from URL
 * 9)  vaccel_resource_init_multi()
 * 10) vaccel_resource_init_from_buf()
 * 11) vaccel_resource_init_from_files()
 * 12) vaccel_resource_regitster()
 * 13) vaccel_resource_unregister()
 * 14) vaccel_resource_release()
 * 15) vaccel_resource_directory()
 * 16) vaccel_session_has_resource()
 *
 *
 */

#include <catch.hpp>
#include <utils.hpp>
#include "fff.h"
DEFINE_FFF_GLOBALS;

#include <vaccel.h>

extern "C" {
FAKE_VALUE_FUNC(int, net_file_download, const char *, const char *);
}

int custom_net_file_download(const char *path, const char *download_path)
{
	char *file_to_copy = abs_path(
		BUILD_ROOT,
		"../examples/models/tf/lstm2/variables/variables.index");
	if (!path || !download_path)
		return VACCEL_EINVAL;

	if (strlen(download_path) + 1 > PATH_MAX) {
		vaccel_error("Path %s name too long", path);
		return VACCEL_ENAMETOOLONG;
	}

	if (fs_path_is_file(download_path))
		return VACCEL_EEXIST;

	if (fs_path_exists(download_path)) {
		vaccel_error("Path %s exists but is not a file", path);
		return VACCEL_EINVAL;
	}

	FILE *fp = fopen(download_path, "wb");
	if (!fp) {
		vaccel_error("Could not open file %s: %s", download_path,
			     strerror(errno));
		return VACCEL_EIO;
	}

	unsigned char *buf;
	size_t len;
	int ret = fs_file_read(file_to_copy, (void **)&buf, &len);
	if (ret) {
		vaccel_error("Custom net download: Could not read %s",
			     file_to_copy);
		return ret;
	}

	size_t bytes_written = fwrite((char *)buf, 1, len, fp);
	if (bytes_written != len) {
		vaccel_error("Could not write to new file %s", download_path);
		return VACCEL_EIO;
	}

	fclose(fp);
	free(file_to_copy);
	free(buf);
	return VACCEL_OK;
}

int string_in_list(const char *target, const char **list, int list_size)
{
	for (int i = 0; i < list_size; i++) {
		if (strcmp(target, list[i]) == 0) {
			return 1;
		}
	}
	return 0;
}

TEST_CASE("vaccel_resource_init from directory", "[resources]")
{
	int ret;
	char *dir_path =
		abs_path(BUILD_ROOT, "../examples/models/tf/lstm2/variables");
	struct vaccel_session sess;
	struct vaccel_resource res;

	/* Session initialization */
	ret = vaccel_session_init(&sess, 0);
	REQUIRE(VACCEL_OK == ret);
	REQUIRE(1 == sess.id);

	/* Resource initialization */
	ret = vaccel_resource_init(&res, dir_path, VACCEL_RESOURCE_DATA);
	REQUIRE(VACCEL_OK == ret);
	REQUIRE(res.id == 1);
	REQUIRE(res.remote_id == -1);
	REQUIRE(res.files == NULL);
	REQUIRE(res.nr_files == 0);
	REQUIRE(res.type == VACCEL_RESOURCE_DATA);
	REQUIRE(res.path_type == VACCEL_PATH_DIR);
	REQUIRE(res.nr_paths == 1);
	REQUIRE(res.rundir == NULL);

	/* Resource registration */
	ret = vaccel_resource_register(&res, &sess);
	REQUIRE(VACCEL_OK == ret);
	REQUIRE((int)res.nr_files == fs_dir_process_files(dir_path, NULL));
	REQUIRE(res.files);

	const char *dir_files[2] = { "variables.data-00000-of-00001",
				     "variables.index" };
	for (size_t i = 0; i < res.nr_files; i++) {
		REQUIRE(res.files[i]);
		REQUIRE(res.files[i]->path);
		REQUIRE(fs_path_is_file(res.files[i]->path));

		char *filename = NULL;
		path_file_name(&filename, res.files[i]->path);
		REQUIRE(string_in_list(filename, dir_files, 2));
		free(filename);
	}

	/* resource directory */
	char dir_resp[256] = { 0 };
	ret = vaccel_resource_directory(&res, dir_resp, sizeof(dir_resp), NULL);
	REQUIRE(0 == strcmp(dir_resp, dir_path));

	/* resource directory with alloc*/
	char *dir_resp_alloc;
	ret = vaccel_resource_directory(&res, NULL, 0, &dir_resp_alloc);
	REQUIRE(0 == strcmp(dir_resp_alloc, dir_path));
	free(dir_resp_alloc);

	/* get by id */
	struct vaccel_resource *found_by_id;
	ret = vaccel_resource_get_by_id(&found_by_id, 1);
	REQUIRE(VACCEL_OK == ret);
	REQUIRE(found_by_id->id == res.id);

	/* Has resource */
	REQUIRE(vaccel_session_has_resource(&sess, &res));

	/* Unregister the resource */
	ret = vaccel_resource_unregister(&res, &sess);
	REQUIRE(VACCEL_OK == ret);
	REQUIRE_FALSE(vaccel_session_has_resource(&sess, &res));

	/* Release the resource */
	ret = vaccel_resource_release(&res);
	REQUIRE(VACCEL_OK == ret);
	REQUIRE(res.rundir == NULL);
	REQUIRE(res.files == NULL);
	REQUIRE(res.nr_files == 0);
	REQUIRE(res.paths == NULL);
	REQUIRE(res.nr_paths == 0);
	REQUIRE(res.id <= 0);

	/* Release the session */
	ret = vaccel_session_release(&sess);
	REQUIRE(VACCEL_OK == ret);
	REQUIRE(sess.id <= 0);

	free(dir_path);
}

TEST_CASE("vaccel_resource_init_multi", "[resources]")
{
	int ret;
	char *file1 = abs_path(
		BUILD_ROOT,
		"../examples/models/tf/lstm2/variables/variables.data-00000-of-00001");
	char *file2 = abs_path(
		BUILD_ROOT,
		"../examples/models/tf/lstm2/variables/variables.index");
	const char *paths[2] = { file1, file2 };

	struct vaccel_session sess;
	struct vaccel_resource res;

	/* Session initialization */
	ret = vaccel_session_init(&sess, 0);
	REQUIRE(VACCEL_OK == ret);
	REQUIRE(1 == sess.id);

	/* Resource initialization */
	ret = vaccel_resource_init_multi(&res, paths,
					 sizeof(paths) / sizeof(char *),
					 VACCEL_RESOURCE_DATA);
	REQUIRE(VACCEL_OK == ret);
	REQUIRE(res.id == 1);
	REQUIRE(res.remote_id == -1);
	REQUIRE(res.type == VACCEL_RESOURCE_DATA);
	REQUIRE(res.path_type == VACCEL_PATH_LOCAL);
	REQUIRE(res.nr_paths == 2);
	REQUIRE(res.paths);
	for (size_t i = 0; i < res.nr_paths; i++) {
		REQUIRE(res.paths[i]);
		REQUIRE(string_in_list(res.paths[i], paths, 2));
	}
	REQUIRE(res.files == NULL);
	REQUIRE(res.nr_files == 0);
	REQUIRE(res.rundir == NULL);

	/* Resource registration */
	ret = vaccel_resource_register(&res, &sess);
	REQUIRE(VACCEL_OK == ret);
	REQUIRE((int)res.nr_files == 2);
	REQUIRE(res.files);

	for (size_t i = 0; i < res.nr_files; i++) {
		REQUIRE(res.files[i]);
		REQUIRE(res.files[i]->path);
		REQUIRE(fs_path_is_file(res.files[i]->path));
		REQUIRE(string_in_list(res.files[i]->path, paths, 2));
	}

	/* get by id */
	struct vaccel_resource *found_by_id;
	ret = vaccel_resource_get_by_id(&found_by_id, 1);
	REQUIRE(VACCEL_OK == ret);
	REQUIRE(found_by_id->id == res.id);

	/* Has resource */
	REQUIRE(vaccel_session_has_resource(&sess, &res));

	/* Unregister the resource */
	ret = vaccel_resource_unregister(&res, &sess);
	REQUIRE(VACCEL_OK == ret);
	REQUIRE_FALSE(vaccel_session_has_resource(&sess, &res));

	/* Release the resource */
	ret = vaccel_resource_release(&res);
	REQUIRE(VACCEL_OK == ret);
	REQUIRE(res.rundir == NULL);
	REQUIRE(res.files == NULL);
	REQUIRE(res.nr_files == 0);
	REQUIRE(res.paths == NULL);
	REQUIRE(res.nr_paths == 0);
	REQUIRE(res.id <= 0);

	/* Release the session */
	ret = vaccel_session_release(&sess);
	REQUIRE(VACCEL_OK == ret);
	REQUIRE(sess.id <= 0);

	free(file1);
	free(file2);
}

TEST_CASE("resource from files", "[resources]")
{
	int ret;
	char *path1 = abs_path(
		BUILD_ROOT,
		"../examples/models/tf/lstm2/variables/variables.data-00000-of-00001");
	char *path2 = abs_path(
		BUILD_ROOT,
		"../examples/models/tf/lstm2/variables/variables.index");

	struct vaccel_resource res;
	struct vaccel_file f1, f2;

	/* init files */
	ret = vaccel_file_init(&f1, path1);
	REQUIRE(VACCEL_OK == ret);

	ret = vaccel_file_init(&f2, path2);
	REQUIRE(VACCEL_OK == ret);

	/* read files */
	ret = vaccel_file_read(&f1);
	REQUIRE(VACCEL_OK == ret);

	ret = vaccel_file_read(&f2);
	REQUIRE(VACCEL_OK == ret);

	const struct vaccel_file *vaccel_files[2] = { &f1, &f2 };

	/* resource init from files */
	ret = vaccel_resource_init_from_files(&res, vaccel_files, 2,
					      VACCEL_RESOURCE_DATA);
	REQUIRE(VACCEL_OK == ret);
	REQUIRE(1 == res.id);
	REQUIRE(-1 == res.remote_id);
	REQUIRE(VACCEL_RESOURCE_DATA == res.type);
	REQUIRE(NULL == res.paths);
	REQUIRE(0 == res.nr_paths);
	REQUIRE(VACCEL_PATH_LOCAL == res.path_type);
	REQUIRE(res.rundir);
	REQUIRE(res.files);
	REQUIRE(2 == res.nr_files);
	REQUIRE(res.files[0]);
	REQUIRE(res.files[1]);

	/* session init */
	struct vaccel_session sess;
	ret = vaccel_session_init(&sess, 0);
	REQUIRE(VACCEL_OK == ret);
	REQUIRE(1 == sess.id);

	/* register resource */
	ret = vaccel_resource_register(&res, &sess);
	REQUIRE(VACCEL_OK == ret);

	/* get by id */
	struct vaccel_resource *found_by_id;
	ret = vaccel_resource_get_by_id(&found_by_id, 1);
	REQUIRE(VACCEL_OK == ret);
	REQUIRE(found_by_id->id == res.id);

	/* Has resource */
	REQUIRE(vaccel_session_has_resource(&sess, &res));

	/* Unregister the resource */
	ret = vaccel_resource_unregister(&res, &sess);
	REQUIRE(VACCEL_OK == ret);
	REQUIRE_FALSE(vaccel_session_has_resource(&sess, &res));

	/* Release the resource */
	ret = vaccel_resource_release(&res);
	REQUIRE(VACCEL_OK == ret);
	REQUIRE(res.rundir == NULL);
	REQUIRE(res.files == NULL);
	REQUIRE(res.nr_files == 0);
	REQUIRE(res.paths == NULL);
	REQUIRE(res.nr_paths == 0);
	REQUIRE(res.id <= 0);

	/* Release the session */
	ret = vaccel_session_release(&sess);
	REQUIRE(VACCEL_OK == ret);
	REQUIRE(sess.id <= 0);

	/* Release files */
	ret = vaccel_file_release(&f1);
	REQUIRE(VACCEL_OK == ret);

	ret = vaccel_file_release(&f2);
	REQUIRE(VACCEL_OK == ret);

	free(path1);
	free(path2);
}

TEST_CASE("vaccel_resource_init_from_buf", "[resources]")
{
	int ret;
	char *file = abs_path(BUILD_ROOT, "examples/libmytestlib.so");

	struct vaccel_session sess;
	struct vaccel_resource res;

	/* Session initialization */
	ret = vaccel_session_init(&sess, 0);
	REQUIRE(VACCEL_OK == ret);
	REQUIRE(1 == sess.id);

	/* Read file */
	size_t len;
	unsigned char *buff;
	ret = fs_file_read(file, (void **)&buff, &len);
	REQUIRE(VACCEL_OK == ret);

	/* Resource initialization */
	ret = vaccel_resource_init_from_buf(&res, buff, len,
					    VACCEL_RESOURCE_LIB, "lib.so");
	REQUIRE(VACCEL_OK == ret);
	REQUIRE(res.id == 1);
	REQUIRE(res.remote_id == -1);
	REQUIRE(res.type == VACCEL_RESOURCE_LIB);
	REQUIRE(res.path_type == VACCEL_PATH_LOCAL);
	REQUIRE(res.nr_paths == 0);
	REQUIRE(res.files);
	REQUIRE(res.files[0]);
	REQUIRE(res.rundir);
	REQUIRE(res.paths == NULL);
	REQUIRE(res.nr_paths == 0);

	/* Resource registration */
	ret = vaccel_resource_register(&res, &sess);
	REQUIRE(VACCEL_OK == ret);

	/* get by id */
	struct vaccel_resource *found_by_id;
	ret = vaccel_resource_get_by_id(&found_by_id, 1);
	REQUIRE(VACCEL_OK == ret);
	REQUIRE(found_by_id->id == res.id);

	/* Has resource */
	REQUIRE(vaccel_session_has_resource(&sess, &res));

	/* Unregister the resource */
	ret = vaccel_resource_unregister(&res, &sess);
	REQUIRE(VACCEL_OK == ret);
	REQUIRE_FALSE(vaccel_session_has_resource(&sess, &res));

	/* Release the resource */
	ret = vaccel_resource_release(&res);
	REQUIRE(VACCEL_OK == ret);
	REQUIRE(res.rundir == NULL);
	REQUIRE(res.files == NULL);
	REQUIRE(res.nr_files == 0);
	REQUIRE(res.paths == NULL);
	REQUIRE(res.nr_paths == 0);
	REQUIRE(res.id <= 0);

	/* Release the session */
	ret = vaccel_session_release(&sess);
	REQUIRE(VACCEL_OK == ret);
	REQUIRE(sess.id <= 0);

	free(buff);
	free(file);
}

TEST_CASE("vaccel_resource_init - from url", "[resources]")
{
	int ret;
	const char *url =
		"https://raw.githubusercontent.com/nubificus/vaccel/main/examples/models/tf/lstm2/variables/variables.index";

	struct vaccel_session sess;
	struct vaccel_resource res;

	net_file_download_fake.custom_fake = custom_net_file_download;

	/* Session initialization */
	ret = vaccel_session_init(&sess, 0);
	REQUIRE(VACCEL_OK == ret);
	REQUIRE(1 == sess.id);

	/* Resource initialization */
	ret = vaccel_resource_init(&res, url, VACCEL_RESOURCE_DATA);
	REQUIRE(VACCEL_OK == ret);
	REQUIRE(res.id == 1);
	REQUIRE(res.remote_id == -1);
	REQUIRE(res.type == VACCEL_RESOURCE_DATA);
	REQUIRE(res.path_type == VACCEL_PATH_REMOTE);
	REQUIRE(res.nr_paths == 1);
	REQUIRE(res.paths);
	REQUIRE(res.paths[0]);
	REQUIRE(res.files == NULL);
	REQUIRE(res.nr_files == 0);
	REQUIRE(res.rundir == NULL);

	/* Resource registration */
	ret = vaccel_resource_register(&res, &sess);
	REQUIRE(VACCEL_OK == ret);
	REQUIRE(res.rundir);
	REQUIRE(res.nr_files == 1);
	REQUIRE(res.files);
	REQUIRE(res.files[0]);
	REQUIRE(res.files[0]->path);

	/* Check content */
	char *file = abs_path(
		BUILD_ROOT,
		"../examples/models/tf/lstm2/variables/variables.index");
	size_t len1, len2;
	unsigned char *buf1, *buf2;
	ret = fs_file_read(file, (void **)&buf1, &len1);
	REQUIRE(VACCEL_OK == ret);
	REQUIRE(buf1);
	REQUIRE(len1 > 0);

	ret = fs_file_read(res.files[0]->path, (void **)&buf2, &len2);
	REQUIRE(VACCEL_OK == ret);
	REQUIRE(buf2);
	REQUIRE(len2 > 0);

	REQUIRE(len1 == len2);
	for (size_t i = 0; i < len1; i++) {
		REQUIRE(buf1[i] == buf2[i]);
	}
	free(file);
	free(buf1);
	free(buf2);

	/* get by id */
	struct vaccel_resource *found_by_id;
	ret = vaccel_resource_get_by_id(&found_by_id, 1);
	REQUIRE(VACCEL_OK == ret);
	REQUIRE(found_by_id->id == res.id);

	/* Has resource */
	REQUIRE(vaccel_session_has_resource(&sess, &res));

	/* Unregister the resource */
	ret = vaccel_resource_unregister(&res, &sess);
	REQUIRE(VACCEL_OK == ret);
	REQUIRE_FALSE(vaccel_session_has_resource(&sess, &res));

	/* Release the resource */
	ret = vaccel_resource_release(&res);
	REQUIRE(VACCEL_OK == ret);
	REQUIRE(res.rundir == NULL);
	REQUIRE(res.files == NULL);
	REQUIRE(res.nr_files == 0);
	REQUIRE(res.paths == NULL);
	REQUIRE(res.nr_paths == 0);
	REQUIRE(res.id <= 0);

	/* Release the session */
	ret = vaccel_session_release(&sess);
	REQUIRE(VACCEL_OK == ret);
	REQUIRE(sess.id <= 0);
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
		ret = vaccel_resource_init(nullptr, nullptr, test_type);
		REQUIRE(ret == VACCEL_EINVAL);

		ret = vaccel_resource_release(nullptr);
		REQUIRE(ret == VACCEL_EINVAL);
	}

	// Test creation and destruction of a valid resource
	SECTION("Valid resource")
	{
		ret = vaccel_resource_init(&res, test_path, test_type);
		REQUIRE(ret == VACCEL_OK);

		REQUIRE(res.id == 1);
		REQUIRE(res.type == VACCEL_RESOURCE_LIB);
		REQUIRE_FALSE(list_empty(&res.entry));
		REQUIRE(res.refcount == 0);
		REQUIRE(res.rundir == NULL);

		ret = vaccel_resource_release(&res);
		REQUIRE(ret == VACCEL_OK);

		REQUIRE(res.id == -1);
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
	ret = vaccel_resource_init(&res, test_path, test_type);
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
	ret = vaccel_resource_release(&res);
	REQUIRE(ret == VACCEL_OK);

	REQUIRE(res.id == -1);
	REQUIRE(list_empty(&res.entry));
	REQUIRE(res.refcount == 0);
	REQUIRE(res.rundir == NULL);

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
	ret = vaccel_resource_init(&test_res, test_path, test_type);
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
	ret = vaccel_resource_release(&test_res);
	REQUIRE(ret == VACCEL_OK);

	REQUIRE(test_res.id == -1);
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

	ret = vaccel_resource_init(&test_res, test_path, test_type);
	REQUIRE(ret == VACCEL_EPERM);

	ret = vaccel_resource_get_by_id(&result_resource, id_to_find);
	REQUIRE(ret == VACCEL_EPERM);

	ret = vaccel_resource_release(&test_res);
	REQUIRE(ret == VACCEL_EPERM);

	ret = resource_create_rundir(nullptr);
	REQUIRE(ret == VACCEL_EINVAL);

	// bootstrap again so the rest of the tests run correctly
	ret = resources_bootstrap();
	REQUIRE(ret == VACCEL_OK);

	free(test_path);
}
