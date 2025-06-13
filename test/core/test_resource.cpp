// SPDX-License-Identifier: Apache-2.0

/*
 * The code below performs unit testing to resources.
 *
 * 1)  resource_bootstrap()
 * 2)  resource_new()
 * 3)  resource_destroy()
 * 4)  resource_create_rundir()
 * 5)  vaccel_resource_get_by_id()
 * 6)  vaccel_resource_init(), from dir
 * 7)  vaccel_resource_init(), from blob
 * 8)  vaccel_resource_init(), from URL
 * 9)  vaccel_resource_init_multi()
 * 10) vaccel_resource_init_from_buf()
 * 11) vaccel_resource_init_from_blobs()
 * 12) vaccel_resource_register()
 * 13) vaccel_resource_unregister()
 * 14) vaccel_resource_release()
 * 15) vaccel_resource_directory()
 * 16) vaccel_session_has_resource()
 * 17) vaccel_resource_get_by_type()
 * 18) vaccel_resource_get_all_by_type()
 *
 */

#include "utils.hpp"
#include "vaccel.h"
#include <catch.hpp>
#include <cerrno>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fff.h>
#include <linux/limits.h>
#include <mock_virtio.hpp>

DEFINE_FFF_GLOBALS;

extern "C" {
FAKE_VALUE_FUNC(struct vaccel_plugin *, plugin_virtio);
FAKE_VALUE_FUNC(int, net_nocurl_file_download, const char *, const char *);
}

#define DOWNLOAD_FILE "examples/models/torch/cnn_trace.pt"

static auto mock_net_nocurl_file_download(const char *path,
					  const char *download_path) -> int
{
	(void)path;

	FILE *fp = fopen(download_path, "wb");
	if (fp == nullptr) {
		vaccel_error("Could not open blob %s: %s", download_path,
			     strerror(errno));
		return VACCEL_EIO;
	}

	char *file_to_copy = abs_path(SOURCE_ROOT, DOWNLOAD_FILE);

	unsigned char *buf;
	size_t len;
	int const ret = fs_file_read(file_to_copy, (void **)&buf, &len);
	if (ret != 0) {
		vaccel_error("Could not read blob %s", file_to_copy);
		return ret;
	}

	size_t const bytes_written = fwrite((char *)buf, 1, len, fp);
	if (bytes_written != len) {
		vaccel_error("Could not write to new blob %s", download_path);
		return VACCEL_EIO;
	}

	fclose(fp);
	free(file_to_copy);
	free(buf);
	return VACCEL_OK;
}

static auto string_in_list(const char *target, const char **list,
			   int list_size) -> int
{
	for (int i = 0; i < list_size; i++) {
		if (strcmp(target, list[i]) == 0) {
			return 1;
		}
	}
	return 0;
}

// Test case for resource from directory path operations
TEST_CASE("resource_from_directory_path", "[core][resource]")
{
	int ret;
	char *dir_path = abs_path(SOURCE_ROOT, "examples/models/tf/lstm2");
	const size_t nr_resources = 2;
	struct vaccel_resource *resources[nr_resources];

	/* Resource init from dir path */
	struct vaccel_resource res;
	ret = vaccel_resource_init(&res, dir_path, VACCEL_RESOURCE_DATA);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(res.id > 0);
	REQUIRE(res.remote_id == -1);
	REQUIRE(res.blobs == nullptr);
	REQUIRE(res.nr_blobs == 0);
	REQUIRE(res.type == VACCEL_RESOURCE_DATA);
	REQUIRE(res.path_type == VACCEL_PATH_LOCAL_DIR);
	REQUIRE(res.nr_paths == 1);
	REQUIRE(res.rundir == nullptr);
	REQUIRE_FALSE(list_empty(&res.entry));
	REQUIRE(res.refcount == 0);
	resources[0] = &res;

	/* Resource new from dir path */
	struct vaccel_resource *alloc_res;
	ret = vaccel_resource_new(&alloc_res, dir_path, VACCEL_RESOURCE_DATA);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(alloc_res->id == res.id + 1);
	REQUIRE(alloc_res->remote_id == -1);
	REQUIRE(alloc_res->blobs == nullptr);
	REQUIRE(alloc_res->nr_blobs == 0);
	REQUIRE(alloc_res->type == VACCEL_RESOURCE_DATA);
	REQUIRE(alloc_res->path_type == VACCEL_PATH_LOCAL_DIR);
	REQUIRE(alloc_res->nr_paths == 1);
	REQUIRE(alloc_res->rundir == nullptr);
	REQUIRE_FALSE(list_empty(&alloc_res->entry));
	REQUIRE(alloc_res->refcount == 0);
	resources[1] = alloc_res;

	/* Session init */
	struct vaccel_session sess;
	REQUIRE(vaccel_session_init(&sess, 0) == VACCEL_OK);

	/* Register resources */
	for (auto &resource : resources) {
		REQUIRE(resource);
		ret = vaccel_resource_register(resource, &sess);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(resource->nr_blobs ==
			(size_t)fs_dir_process_files(dir_path, nullptr));
		REQUIRE(resource->blobs);
		REQUIRE(resource->refcount == 1);
	}

	const char *dir_files[4] = { "keras_metadata.pb", "saved_model.pb",
				     "variables.data-00000-of-00001",
				     "variables.index" };
	for (auto &resource : resources) {
		/* Check blobs */
		for (size_t j = 0; j < resource->nr_blobs; j++) {
			REQUIRE(resource->blobs[j]);
			REQUIRE(resource->blobs[j]->path);
			REQUIRE(resource->blobs[j]->type == VACCEL_BLOB_FILE);
			REQUIRE_FALSE(resource->blobs[j]->path_owned);
			REQUIRE_FALSE(resource->blobs[j]->data_owned);
			REQUIRE(fs_path_is_file(resource->blobs[j]->path));

			char filename[NAME_MAX];
			path_file_name(resource->blobs[j]->path, filename,
				       NAME_MAX, nullptr);
			REQUIRE(string_in_list(filename, dir_files, 4));
		}

		/* Get resource directory */
		char dir_resp[PATH_MAX] = { '\0' };
		ret = vaccel_resource_directory(resource, dir_resp,
						sizeof(dir_resp), nullptr);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(strcmp(dir_resp, dir_path) == 0);

		/* Get resource directory with alloc */
		char *dir_resp_alloc;
		ret = vaccel_resource_directory(resource, nullptr, 0,
						&dir_resp_alloc);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(strcmp(dir_resp_alloc, dir_path) == 0);
		free(dir_resp_alloc);

		/* Get by id */
		struct vaccel_resource *found_by_id;
		ret = vaccel_resource_get_by_id(&found_by_id, resource->id);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(found_by_id == resource);

		/* Has resource */
		REQUIRE(vaccel_session_has_resource(&sess, resource));
	}

	SECTION("virtio session")
	{
		RESET_FAKE(plugin_virtio);

		plugin_virtio_fake.custom_fake = mock_virtio_plugin_virtio;

		/* Session init */
		struct vaccel_session vsess;
		REQUIRE(vaccel_session_init(&vsess, 0 | VACCEL_PLUGIN_REMOTE) ==
			VACCEL_OK);

		/* Register resource */
		ret = vaccel_resource_register(&res, &vsess);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(res.nr_blobs ==
			(size_t)fs_dir_process_files(dir_path, nullptr));
		REQUIRE(res.blobs);
		REQUIRE(res.refcount == 2);
		REQUIRE(res.id > 0);
		REQUIRE(res.remote_id == 1);

		/* Check blobs */
		const char *dir_files[4] = { "keras_metadata.pb",
					     "saved_model.pb",
					     "variables.data-00000-of-00001",
					     "variables.index" };
		for (size_t i = 0; i < res.nr_blobs; i++) {
			REQUIRE(res.blobs[i]);
			REQUIRE(res.blobs[i]->path);
			REQUIRE(res.blobs[i]->data);
			REQUIRE(res.blobs[i]->type == VACCEL_BLOB_MAPPED);
			REQUIRE_FALSE(res.blobs[i]->path_owned);
			REQUIRE_FALSE(res.blobs[i]->data_owned);
			REQUIRE(string_in_list(res.blobs[i]->name, dir_files,
					       4));
			size_t len;
			unsigned char *buf;
			ret = fs_file_read(res.blobs[i]->path, (void **)&buf,
					   &len);
			REQUIRE(ret == VACCEL_OK);

			REQUIRE(res.blobs[i]->size == len);
			for (size_t j = 0; j < len; j++) {
				REQUIRE(res.blobs[i]->data[j] == buf[j]);
			}
			free(buf);
		}

		/* Get by id */
		struct vaccel_resource *found_by_id;
		ret = vaccel_resource_get_by_id(&found_by_id, res.id);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(found_by_id == &res);

		/* Has resource */
		REQUIRE(vaccel_session_has_resource(&vsess, &res));

		/* Unregister resource */
		ret = vaccel_resource_unregister(&res, &vsess);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE_FALSE(vaccel_session_has_resource(&vsess, &res));

		/* Release session */
		REQUIRE(vaccel_session_release(&vsess) == VACCEL_OK);

		REQUIRE(plugin_virtio_fake.call_count == 4);
	}

	/* Unregister resources */
	for (auto &resource : resources) {
		ret = vaccel_resource_unregister(resource, &sess);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE_FALSE(vaccel_session_has_resource(&sess, resource));
	}

	/* Release session */
	REQUIRE(vaccel_session_release(&sess) == VACCEL_OK);

	/* Free resource */
	ret = vaccel_resource_delete(alloc_res);
	REQUIRE(ret == VACCEL_OK);

	/* Release resource */
	ret = vaccel_resource_release(&res);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(res.rundir == nullptr);
	REQUIRE(res.blobs == nullptr);
	REQUIRE(res.nr_blobs == 0);
	REQUIRE(res.paths == nullptr);
	REQUIRE(res.nr_paths == 0);
	REQUIRE(res.id <= 0);

	free(dir_path);
}

// Test case for resource from blob paths operations
TEST_CASE("resource_from_file_paths", "[core][resource]")
{
	int ret;
	char *file1 = abs_path(
		SOURCE_ROOT,
		"examples/models/tf/lstm2/variables/variables.data-00000-of-00001");
	char *file2 =
		abs_path(SOURCE_ROOT,
			 "examples/models/tf/lstm2/variables/variables.index");
	const char *paths[2] = { file1, file2 };
	const size_t nr_resources = 2;
	struct vaccel_resource *resources[nr_resources];

	/* Resource init from blob paths */
	struct vaccel_resource res;
	ret = vaccel_resource_init_multi(&res, paths,
					 sizeof(paths) / sizeof(char *),
					 VACCEL_RESOURCE_DATA);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(res.id > 0);
	REQUIRE(res.remote_id == -1);
	REQUIRE(res.type == VACCEL_RESOURCE_DATA);
	REQUIRE(res.path_type == VACCEL_PATH_LOCAL_FILE);
	REQUIRE(res.nr_paths == 2);
	REQUIRE(res.paths);
	for (size_t i = 0; i < res.nr_paths; i++) {
		REQUIRE(res.paths[i]);
		REQUIRE(string_in_list(res.paths[i], paths, 2));
	}
	REQUIRE(res.blobs == nullptr);
	REQUIRE(res.nr_blobs == 0);
	REQUIRE(res.rundir == nullptr);
	REQUIRE_FALSE(list_empty(&res.entry));
	REQUIRE(res.refcount == 0);
	resources[0] = &res;

	/* Resource new from blob paths */
	struct vaccel_resource *alloc_res;
	ret = vaccel_resource_multi_new(&alloc_res, paths,
					sizeof(paths) / sizeof(char *),
					VACCEL_RESOURCE_DATA);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(alloc_res->id == res.id + 1);
	REQUIRE(alloc_res->remote_id == -1);
	REQUIRE(alloc_res->type == VACCEL_RESOURCE_DATA);
	REQUIRE(alloc_res->path_type == VACCEL_PATH_LOCAL_FILE);
	REQUIRE(alloc_res->nr_paths == 2);
	REQUIRE(alloc_res->paths);
	for (size_t i = 0; i < alloc_res->nr_paths; i++) {
		REQUIRE(alloc_res->paths[i]);
		REQUIRE(string_in_list(alloc_res->paths[i], paths, 2));
	}
	REQUIRE(alloc_res->blobs == nullptr);
	REQUIRE(alloc_res->nr_blobs == 0);
	REQUIRE(alloc_res->rundir == nullptr);
	REQUIRE_FALSE(list_empty(&alloc_res->entry));
	REQUIRE(alloc_res->refcount == 0);
	resources[1] = alloc_res;

	/* Session init */
	struct vaccel_session sess;
	REQUIRE(vaccel_session_init(&sess, 0) == VACCEL_OK);

	/* Register resources */
	for (auto &resource : resources) {
		ret = vaccel_resource_register(resource, &sess);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(resource->nr_blobs == 2);
		REQUIRE(resource->blobs);
		REQUIRE(resource->refcount == 1);
	}

	for (auto &resource : resources) {
		/* Check blobs */
		for (size_t j = 0; j < resource->nr_blobs; j++) {
			REQUIRE(resource->blobs[j]);
			REQUIRE(resource->blobs[j]->path);
			REQUIRE(resource->blobs[j]->type == VACCEL_BLOB_FILE);
			REQUIRE_FALSE(resource->blobs[j]->path_owned);
			REQUIRE_FALSE(resource->blobs[j]->data_owned);
			REQUIRE(fs_path_is_file(resource->blobs[j]->path));
			REQUIRE(string_in_list(resource->blobs[j]->path, paths,
					       2));
		}

		/* Get by id */
		struct vaccel_resource *found_by_id;
		ret = vaccel_resource_get_by_id(&found_by_id, resource->id);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(found_by_id == resource);

		/* Has resource */
		REQUIRE(vaccel_session_has_resource(&sess, resource));
	}

	SECTION("virtio session")
	{
		RESET_FAKE(plugin_virtio);

		plugin_virtio_fake.custom_fake = mock_virtio_plugin_virtio;

		/* Session init */
		struct vaccel_session vsess;
		REQUIRE(vaccel_session_init(&vsess, 0 | VACCEL_PLUGIN_REMOTE) ==
			VACCEL_OK);

		/* Register resource */
		ret = vaccel_resource_register(&res, &vsess);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(res.nr_blobs == 2);
		REQUIRE(res.blobs);
		REQUIRE(res.refcount == 2);
		REQUIRE(res.id > 0);
		REQUIRE(res.remote_id == 1);

		/* Check blobs */
		for (size_t i = 0; i < res.nr_blobs; i++) {
			REQUIRE(res.blobs[i]);
			REQUIRE(res.blobs[i]->path);
			REQUIRE(res.blobs[i]->data);
			REQUIRE(res.blobs[i]->type == VACCEL_BLOB_MAPPED);
			REQUIRE_FALSE(res.blobs[i]->path_owned);
			REQUIRE_FALSE(res.blobs[i]->data_owned);
			REQUIRE(string_in_list(res.blobs[i]->path, paths, 2));

			size_t len;
			unsigned char *buf;
			ret = fs_file_read(res.blobs[i]->path, (void **)&buf,
					   &len);
			REQUIRE(ret == VACCEL_OK);

			REQUIRE(res.blobs[i]->size == len);
			for (size_t j = 0; j < len; j++) {
				REQUIRE(res.blobs[i]->data[j] == buf[j]);
			}
			free(buf);
		}

		/* Get by id */
		struct vaccel_resource *found_by_id;
		ret = vaccel_resource_get_by_id(&found_by_id, res.id);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(found_by_id == &res);

		/* Has resource */
		REQUIRE(vaccel_session_has_resource(&vsess, &res));

		/* Unregister resource */
		ret = vaccel_resource_unregister(&res, &vsess);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE_FALSE(vaccel_session_has_resource(&vsess, &res));

		/* Release session */
		REQUIRE(vaccel_session_release(&vsess) == VACCEL_OK);

		REQUIRE(plugin_virtio_fake.call_count == 4);
	}

	/* Unregister resources */
	for (auto &resource : resources) {
		ret = vaccel_resource_unregister(resource, &sess);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE_FALSE(vaccel_session_has_resource(&sess, resource));
	}

	/* Release session */
	REQUIRE(vaccel_session_release(&sess) == VACCEL_OK);

	/* Free resource */
	ret = vaccel_resource_delete(alloc_res);
	REQUIRE(ret == VACCEL_OK);

	/* Release resource */
	ret = vaccel_resource_release(&res);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(res.rundir == nullptr);
	REQUIRE(res.blobs == nullptr);
	REQUIRE(res.nr_blobs == 0);
	REQUIRE(res.paths == nullptr);
	REQUIRE(res.nr_paths == 0);
	REQUIRE(res.id <= 0);

	free(file1);
	free(file2);
}

// Test case for resource from url path operations
TEST_CASE("resource_from_url_path", "[core][resource]")
{
	int ret;
	char url[PATH_MAX];
	const size_t nr_resources = 2;
	struct vaccel_resource *resources[nr_resources];

	REQUIRE(path_init_from_parts(url, PATH_MAX, REPO_RAWURL, DOWNLOAD_FILE,
				     nullptr) == VACCEL_OK);

	net_nocurl_file_download_fake.custom_fake =
		mock_net_nocurl_file_download;

	/* Resource init from url */
	struct vaccel_resource res;
	ret = vaccel_resource_init(&res, url, VACCEL_RESOURCE_DATA);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(res.id > 0);
	REQUIRE(res.remote_id == -1);
	REQUIRE(res.type == VACCEL_RESOURCE_DATA);
	REQUIRE(res.path_type == VACCEL_PATH_REMOTE_FILE);
	REQUIRE(res.nr_paths == 1);
	REQUIRE(res.paths);
	REQUIRE(res.paths[0]);
	REQUIRE(res.blobs == nullptr);
	REQUIRE(res.nr_blobs == 0);
	REQUIRE(res.rundir == nullptr);
	REQUIRE_FALSE(list_empty(&res.entry));
	REQUIRE(res.refcount == 0);
	resources[0] = &res;

	/* Resource init from url */
	struct vaccel_resource *alloc_res;
	ret = vaccel_resource_new(&alloc_res, url, VACCEL_RESOURCE_DATA);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(alloc_res->id == res.id + 1);
	REQUIRE(alloc_res->remote_id == -1);
	REQUIRE(alloc_res->type == VACCEL_RESOURCE_DATA);
	REQUIRE(alloc_res->path_type == VACCEL_PATH_REMOTE_FILE);
	REQUIRE(alloc_res->nr_paths == 1);
	REQUIRE(alloc_res->paths);
	REQUIRE(alloc_res->paths[0]);
	REQUIRE(alloc_res->blobs == nullptr);
	REQUIRE(alloc_res->nr_blobs == 0);
	REQUIRE(alloc_res->rundir == nullptr);
	REQUIRE_FALSE(list_empty(&alloc_res->entry));
	REQUIRE(alloc_res->refcount == 0);
	resources[1] = alloc_res;

	/* Session init */
	struct vaccel_session sess;
	REQUIRE(vaccel_session_init(&sess, 0) == VACCEL_OK);

	SECTION("virtio session")
	{
		RESET_FAKE(plugin_virtio);

		plugin_virtio_fake.custom_fake = mock_virtio_plugin_virtio;

		/* Session init */
		struct vaccel_session vsess;
		REQUIRE(vaccel_session_init(&vsess, 0 | VACCEL_PLUGIN_REMOTE) ==
			VACCEL_OK);

		/* Register resource */
		ret = vaccel_resource_register(&res, &vsess);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(res.nr_blobs == 0);
		REQUIRE_FALSE(res.blobs);
		REQUIRE(res.refcount == 1);
		REQUIRE(res.id > 0);
		REQUIRE(res.remote_id == 1);

		/* Get by id */
		struct vaccel_resource *found_by_id;
		ret = vaccel_resource_get_by_id(&found_by_id, res.id);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(found_by_id == &res);

		/* Has resource */
		REQUIRE(vaccel_session_has_resource(&vsess, &res));

		/* Unregister resource */
		ret = vaccel_resource_unregister(&res, &vsess);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE_FALSE(vaccel_session_has_resource(&vsess, &res));

		/* Release session */
		REQUIRE(vaccel_session_release(&vsess) == VACCEL_OK);

		REQUIRE(plugin_virtio_fake.call_count == 4);
	}

	/* Register resources */
	for (auto &resource : resources) {
		ret = vaccel_resource_register(resource, &sess);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(resource->rundir);
		REQUIRE(resource->nr_blobs == 1);
		REQUIRE(resource->blobs);
		REQUIRE(resource->blobs[0]);
		REQUIRE(resource->blobs[0]->path);
		REQUIRE(resource->blobs[0]->type == VACCEL_BLOB_FILE);
		REQUIRE(resource->blobs[0]->path_owned);
		REQUIRE_FALSE(resource->blobs[0]->data_owned);
		REQUIRE(resource->refcount == 1);
	}

	char *file = abs_path(SOURCE_ROOT, DOWNLOAD_FILE);
	size_t len1;
	unsigned char *buf1;
	ret = fs_file_read(file, (void **)&buf1, &len1);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(buf1);
	REQUIRE(len1 > 0);
	for (auto &resource : resources) {
		/* Check content */
		size_t len2;
		unsigned char *buf2;
		ret = fs_file_read(resource->blobs[0]->path, (void **)&buf2,
				   &len2);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(buf2);
		REQUIRE(len2 > 0);
		free(buf2);

		/* Get by id */
		struct vaccel_resource *found_by_id;
		ret = vaccel_resource_get_by_id(&found_by_id, resource->id);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(found_by_id == resource);

		/* Has resource */
		REQUIRE(vaccel_session_has_resource(&sess, resource));
	}
	free(file);
	free(buf1);

	/* Unregister resources */
	for (auto &resource : resources) {
		ret = vaccel_resource_unregister(resource, &sess);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE_FALSE(vaccel_session_has_resource(&sess, resource));
		REQUIRE(res.refcount == 0);
	}

	/* Release session */
	REQUIRE(vaccel_session_release(&sess) == VACCEL_OK);

	/* Free resource */
	ret = vaccel_resource_delete(alloc_res);
	REQUIRE(ret == VACCEL_OK);

	/* Release resource */
	ret = vaccel_resource_release(&res);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(res.rundir == nullptr);
	REQUIRE(res.blobs == nullptr);
	REQUIRE(res.nr_blobs == 0);
	REQUIRE(res.paths == nullptr);
	REQUIRE(res.nr_paths == 0);
	REQUIRE(res.id <= 0);
}

// Test case for resource from buffer operations
TEST_CASE("resource_from_buffer", "[core][resource]")
{
	int ret;
	char *file = abs_path(BUILD_ROOT, "examples/libmytestlib.so");
	const size_t nr_resources = 2;
	struct vaccel_resource *resources[nr_resources];
	struct vaccel_session sess;

	/* Session init */
	REQUIRE(vaccel_session_init(&sess, 0) == VACCEL_OK);

	/* Read blob */
	size_t len;
	unsigned char *buff;
	ret = fs_file_read(file, (void **)&buff, &len);
	REQUIRE(ret == VACCEL_OK);

	/* Resource init from buffer */
	struct vaccel_resource res;
	ret = vaccel_resource_init_from_buf(
		&res, buff, len, VACCEL_RESOURCE_LIB, "lib.so", false);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(res.id > 0);
	REQUIRE(res.remote_id == -1);
	REQUIRE(res.type == VACCEL_RESOURCE_LIB);
	REQUIRE(res.path_type == VACCEL_PATH_LOCAL_FILE);
	REQUIRE(res.nr_paths == 0);
	REQUIRE(res.blobs);
	REQUIRE(res.blobs[0]);
	REQUIRE(res.blobs[0]->type == VACCEL_BLOB_MAPPED);
	REQUIRE(res.blobs[0]->data);
	REQUIRE(res.blobs[0]->data != buff);
	REQUIRE(res.blobs[0]->size == len);
	REQUIRE(res.blobs[0]->data_owned);
	REQUIRE(res.blobs[0]->path_owned);
	REQUIRE(res.rundir);
	REQUIRE(res.paths == nullptr);
	REQUIRE(res.nr_paths == 0);
	REQUIRE_FALSE(list_empty(&res.entry));
	REQUIRE(res.refcount == 0);
	resources[0] = &res;

	/* Resource new from buffer */
	struct vaccel_resource *alloc_res;
	ret = vaccel_resource_from_buf(&alloc_res, buff, len,
				       VACCEL_RESOURCE_LIB, "lib.so", false);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(alloc_res->id == res.id + 1);
	REQUIRE(alloc_res->remote_id == -1);
	REQUIRE(alloc_res->type == VACCEL_RESOURCE_LIB);
	REQUIRE(alloc_res->path_type == VACCEL_PATH_LOCAL_FILE);
	REQUIRE(alloc_res->nr_paths == 0);
	REQUIRE(alloc_res->blobs);
	REQUIRE(alloc_res->blobs[0]);
	REQUIRE(alloc_res->blobs[0]->type == VACCEL_BLOB_MAPPED);
	REQUIRE(alloc_res->blobs[0]->data);
	REQUIRE(alloc_res->blobs[0]->data != buff);
	REQUIRE(alloc_res->blobs[0]->size == len);
	REQUIRE(alloc_res->blobs[0]->data_owned);
	REQUIRE(alloc_res->blobs[0]->path_owned);
	REQUIRE(alloc_res->rundir);
	REQUIRE(alloc_res->paths == nullptr);
	REQUIRE(alloc_res->nr_paths == 0);
	REQUIRE_FALSE(list_empty(&alloc_res->entry));
	REQUIRE(alloc_res->refcount == 0);
	resources[1] = alloc_res;

	for (auto &resource : resources) {
		/* Register resources */
		ret = vaccel_resource_register(resource, &sess);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(resource->refcount == 1);

		/* Get by id */
		struct vaccel_resource *found_by_id;
		ret = vaccel_resource_get_by_id(&found_by_id, resource->id);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(found_by_id == resource);

		/* Has resource */
		REQUIRE(vaccel_session_has_resource(&sess, resource));

		/* Unregister resources */
		ret = vaccel_resource_unregister(resource, &sess);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE_FALSE(vaccel_session_has_resource(&sess, resource));
		REQUIRE(resource->refcount == 0);
	}

	/* Free resource */
	ret = vaccel_resource_delete(alloc_res);
	REQUIRE(ret == VACCEL_OK);

	/* Release resource */
	ret = vaccel_resource_release(&res);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(res.rundir == nullptr);
	REQUIRE(res.blobs == nullptr);
	REQUIRE(res.nr_blobs == 0);
	REQUIRE(res.paths == nullptr);
	REQUIRE(res.nr_paths == 0);
	REQUIRE(res.id <= 0);

	/* Release session */
	REQUIRE(vaccel_session_release(&sess) == VACCEL_OK);

	free(buff);
	free(file);
}

TEST_CASE("resource_from_blobs_mem_only", "[core][resource]")
{
	int ret;
	struct vaccel_blob b1;
	struct vaccel_blob b2;
	char *path1 = abs_path(
		SOURCE_ROOT,
		"examples/models/tf/lstm2/variables/variables.data-00000-of-00001");
	char *path2 =
		abs_path(SOURCE_ROOT,
			 "examples/models/tf/lstm2/variables/variables.index");

	/* Read blob */
	size_t len1;
	size_t len2;
	unsigned char *buff1;
	unsigned char *buff2;
	ret = fs_file_read(path1, (void **)&buff1, &len1);
	REQUIRE(ret == VACCEL_OK);

	ret = fs_file_read(path2, (void **)&buff2, &len2);
	REQUIRE(ret == VACCEL_OK);

	/* Init blobs */
	ret = vaccel_blob_init_from_buf(&b1, buff1, len1, true, "buf", nullptr,
					true);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(b1.type == VACCEL_BLOB_BUFFER);
	REQUIRE(b1.data_owned);
	REQUIRE_FALSE(b1.path_owned);
	REQUIRE(b1.data);
	REQUIRE(b1.size == len1);

	ret = vaccel_blob_init_from_buf(&b2, buff2, len2, true, "buf", nullptr,
					true);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(b2.type == VACCEL_BLOB_BUFFER);
	REQUIRE(b2.data_owned);
	REQUIRE_FALSE(b2.path_owned);
	REQUIRE(b2.data);
	REQUIRE(b2.size == len2);

	const struct vaccel_blob *vaccel_blobs[2] = { &b1, &b2 };

	/* Resource init from blobs */
	struct vaccel_resource res;
	ret = vaccel_resource_init_from_blobs(&res, vaccel_blobs, 2,
					      VACCEL_RESOURCE_DATA);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(res.id > 0);
	REQUIRE(res.remote_id == -1);
	REQUIRE(res.type == VACCEL_RESOURCE_DATA);
	REQUIRE(res.path_type == VACCEL_PATH_LOCAL_FILE);
	REQUIRE(res.paths == nullptr);
	REQUIRE(res.nr_paths == 0);
	REQUIRE(res.rundir == nullptr);
	REQUIRE(res.blobs);
	REQUIRE(res.nr_blobs == 2);
	REQUIRE_FALSE(list_empty(&res.entry));
	REQUIRE(res.refcount == 0);
	for (size_t i = 0; i != res.nr_blobs; ++i) {
		REQUIRE(res.blobs[i]);
		REQUIRE(res.blobs[i]->type == VACCEL_BLOB_BUFFER);
		REQUIRE(res.blobs[i]->path == nullptr);
		REQUIRE_FALSE(res.blobs[i]->path_owned);
		REQUIRE(res.blobs[i]->data_owned ==
			vaccel_blobs[i]->data_owned);
		REQUIRE(res.blobs[i]->data);
		REQUIRE(res.blobs[i]->size);
	}

	/* Session init */
	struct vaccel_session sess;
	REQUIRE(vaccel_session_init(&sess, 0) == VACCEL_OK);

	/* Register resource */
	ret = vaccel_resource_register(&res, &sess);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(res.refcount == 1);

	/* Get by id */
	struct vaccel_resource *found_by_id;
	ret = vaccel_resource_get_by_id(&found_by_id, res.id);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(found_by_id == &res);

	/* Requirements after registering */
	REQUIRE(res.type == VACCEL_RESOURCE_DATA);
	REQUIRE(res.path_type == VACCEL_PATH_LOCAL_FILE);
	REQUIRE(res.paths == nullptr);
	REQUIRE(res.nr_paths == 0);
	REQUIRE(res.rundir == nullptr);
	REQUIRE(res.blobs);
	REQUIRE(res.nr_blobs == 2);
	REQUIRE_FALSE(list_empty(&res.entry));
	for (size_t i = 0; i != res.nr_blobs; ++i) {
		REQUIRE(res.blobs[i]);
		REQUIRE(res.blobs[i]->type == VACCEL_BLOB_BUFFER);
		REQUIRE(res.blobs[i]->path == nullptr);
		REQUIRE_FALSE(res.blobs[i]->path_owned);
		REQUIRE(res.blobs[i]->data_owned ==
			vaccel_blobs[i]->data_owned);
		REQUIRE(res.blobs[i]->data);
		REQUIRE(res.blobs[i]->size);
	}

	/* Has resource */
	REQUIRE(vaccel_session_has_resource(&sess, &res));

	/* Unregister resource */
	ret = vaccel_resource_unregister(&res, &sess);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE_FALSE(vaccel_session_has_resource(&sess, &res));
	REQUIRE(res.refcount == 0);

	/* Release resource */
	ret = vaccel_resource_release(&res);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(res.rundir == nullptr);
	REQUIRE(res.blobs == nullptr);
	REQUIRE(res.nr_blobs == 0);
	REQUIRE(res.paths == nullptr);
	REQUIRE(res.nr_paths == 0);
	REQUIRE(res.id <= 0);

	/* Release session */
	REQUIRE(vaccel_session_release(&sess) == VACCEL_OK);

	/* Release blobs */
	ret = vaccel_blob_release(&b1);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(b1.type == VACCEL_BLOB_MAX);

	ret = vaccel_blob_release(&b2);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(b2.type == VACCEL_BLOB_MAX);

	free(buff1);
	free(buff2);
	free(path1);
	free(path2);
}

// Test case for resource from vaccel blobs operations
TEST_CASE("resource_from_blobs", "[core][resource]")
{
	int ret;
	const size_t nr_resources = 2;
	struct vaccel_resource *resources[nr_resources];
	struct vaccel_blob b1;
	struct vaccel_blob b2;
	char *path1 = abs_path(
		SOURCE_ROOT,
		"examples/models/tf/lstm2/variables/variables.data-00000-of-00001");
	char *path2 =
		abs_path(SOURCE_ROOT,
			 "examples/models/tf/lstm2/variables/variables.index");

	/* Init blobs */
	ret = vaccel_blob_init(&b1, path1);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(b1.type == VACCEL_BLOB_FILE);
	REQUIRE_FALSE(b1.path_owned);
	REQUIRE_FALSE(b1.data_owned);

	ret = vaccel_blob_init(&b2, path2);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(b2.type == VACCEL_BLOB_FILE);
	REQUIRE_FALSE(b2.path_owned);
	REQUIRE_FALSE(b2.data_owned);

	/* Read blobs */
	ret = vaccel_blob_read(&b1);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(b1.type == VACCEL_BLOB_MAPPED);
	REQUIRE_FALSE(b1.path_owned);
	REQUIRE_FALSE(b1.data_owned);

	ret = vaccel_blob_read(&b2);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(b2.type == VACCEL_BLOB_MAPPED);
	REQUIRE_FALSE(b2.path_owned);
	REQUIRE_FALSE(b2.data_owned);

	const struct vaccel_blob *vaccel_blobs[2] = { &b1, &b2 };

	/* Resource init from blobs */
	struct vaccel_resource res;
	ret = vaccel_resource_init_from_blobs(&res, vaccel_blobs, 2,
					      VACCEL_RESOURCE_DATA);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(res.id > 0);
	REQUIRE(res.remote_id == -1);
	REQUIRE(res.type == VACCEL_RESOURCE_DATA);
	REQUIRE(res.path_type == VACCEL_PATH_LOCAL_FILE);
	REQUIRE(res.paths == nullptr);
	REQUIRE(res.nr_paths == 0);
	REQUIRE(res.rundir);
	REQUIRE(res.blobs);
	REQUIRE(res.nr_blobs == 2);
	REQUIRE_FALSE(list_empty(&res.entry));
	REQUIRE(res.refcount == 0);
	for (size_t i = 0; i != res.nr_blobs; ++i) {
		REQUIRE(res.blobs[i]);
		REQUIRE(res.blobs[i]->type == VACCEL_BLOB_MAPPED);
		REQUIRE(res.blobs[i]->path);
		REQUIRE(res.blobs[i]->path_owned);
		REQUIRE(res.blobs[i]->data_owned);
		REQUIRE(res.blobs[i]->data);
		REQUIRE(res.blobs[i]->size);
	}
	resources[0] = &res;

	/* Resource new from blobs */
	struct vaccel_resource *alloc_res;
	ret = vaccel_resource_from_blobs(&alloc_res, vaccel_blobs, 2,
					 VACCEL_RESOURCE_DATA);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(alloc_res->id == res.id + 1);
	REQUIRE(alloc_res->remote_id == -1);
	REQUIRE(alloc_res->type == VACCEL_RESOURCE_DATA);
	REQUIRE(alloc_res->path_type == VACCEL_PATH_LOCAL_FILE);
	REQUIRE(alloc_res->paths == nullptr);
	REQUIRE(alloc_res->nr_paths == 0);
	REQUIRE(alloc_res->rundir);
	REQUIRE(alloc_res->blobs);
	REQUIRE(alloc_res->nr_blobs == 2);
	REQUIRE_FALSE(list_empty(&alloc_res->entry));
	REQUIRE(alloc_res->refcount == 0);
	for (size_t i = 0; i != alloc_res->nr_blobs; ++i) {
		REQUIRE(alloc_res->blobs[i]);
		REQUIRE(alloc_res->blobs[i]->type == VACCEL_BLOB_MAPPED);
		REQUIRE(alloc_res->blobs[i]->path);
		REQUIRE(alloc_res->blobs[i]->path_owned);
		REQUIRE(alloc_res->blobs[i]->data_owned);
		REQUIRE(alloc_res->blobs[i]->data);
		REQUIRE(alloc_res->blobs[i]->size);
	}
	resources[1] = alloc_res;

	/* Session init */
	struct vaccel_session sess;
	REQUIRE(vaccel_session_init(&sess, 0) == VACCEL_OK);

	for (auto &resource : resources) {
		/* Register resources */
		ret = vaccel_resource_register(resource, &sess);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(resource->refcount == 1);

		/* Get by id */
		struct vaccel_resource *found_by_id;
		ret = vaccel_resource_get_by_id(&found_by_id, resource->id);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(found_by_id == resource);

		/* Has resource */
		REQUIRE(vaccel_session_has_resource(&sess, resource));

		/* Unregister resources */
		ret = vaccel_resource_unregister(resource, &sess);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE_FALSE(vaccel_session_has_resource(&sess, resource));
		REQUIRE(resource->refcount == 0);
	}

	/* Free resource */
	ret = vaccel_resource_delete(alloc_res);
	REQUIRE(ret == VACCEL_OK);

	/* Release resource */
	ret = vaccel_resource_release(&res);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(res.rundir == nullptr);
	REQUIRE(res.blobs == nullptr);
	REQUIRE(res.nr_blobs == 0);
	REQUIRE(res.paths == nullptr);
	REQUIRE(res.nr_paths == 0);
	REQUIRE(res.id <= 0);

	/* Release session */
	REQUIRE(vaccel_session_release(&sess) == VACCEL_OK);

	/* Release blobs */
	ret = vaccel_blob_release(&b1);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(b1.type == VACCEL_BLOB_MAX);

	ret = vaccel_blob_release(&b2);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(b2.type == VACCEL_BLOB_MAX);

	free(path1);
	free(path2);
}

// Test case for resource init failures
TEST_CASE("resource_init_fail", "[core][resource]")
{
	int ret;
	struct vaccel_resource res;
	struct vaccel_resource *alloc_res;
	vaccel_resource_type_t const test_type = VACCEL_RESOURCE_LIB;

	char *test_path = abs_path(BUILD_ROOT, "examples/libmytestlib.so");
	const char *paths[2] = { test_path, nullptr };

	size_t len;
	unsigned char *buff;
	ret = fs_file_read(test_path, (void **)&buff, &len);
	REQUIRE(ret == VACCEL_OK);

	struct vaccel_blob b;
	ret = vaccel_blob_init(&b, test_path);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(b.type == VACCEL_BLOB_FILE);
	REQUIRE_FALSE(b.path_owned);
	REQUIRE_FALSE(b.data_owned);
	REQUIRE(b.data == nullptr);
	REQUIRE(b.size == 0);

	ret = vaccel_blob_read(&b);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(b.type == VACCEL_BLOB_MAPPED);
	REQUIRE_FALSE(b.path_owned);
	REQUIRE_FALSE(b.data_owned);
	REQUIRE(b.data);
	REQUIRE(b.size);

	const struct vaccel_blob *blobs[2] = { &b, nullptr };

	SECTION("init invalid arguments")
	{
		ret = vaccel_resource_init(nullptr, test_path, test_type);
		REQUIRE(ret == VACCEL_EINVAL);
		ret = vaccel_resource_init(&res, nullptr, test_type);
		REQUIRE(ret == VACCEL_EINVAL);
		ret = vaccel_resource_init(&res, nullptr, VACCEL_RESOURCE_MAX);
		REQUIRE(ret == VACCEL_EINVAL);

		ret = vaccel_resource_init_multi(nullptr, paths, 1, test_type);
		REQUIRE(ret == VACCEL_EINVAL);
		ret = vaccel_resource_init_multi(&res, nullptr, 1, test_type);
		REQUIRE(ret == VACCEL_EINVAL);
		ret = vaccel_resource_init_multi(&res, paths, 0, test_type);
		REQUIRE(ret == VACCEL_EINVAL);
		ret = vaccel_resource_init_multi(&res, paths, 1,
						 VACCEL_RESOURCE_MAX);
		REQUIRE(ret == VACCEL_EINVAL);

		ret = vaccel_resource_init_from_buf(nullptr, &buff, len,
						    test_type, nullptr, false);
		REQUIRE(ret == VACCEL_EINVAL);
		ret = vaccel_resource_init_from_buf(&res, nullptr, len,
						    test_type, nullptr, false);
		REQUIRE(ret == VACCEL_EINVAL);
		ret = vaccel_resource_init_from_buf(&res, &buff, 0, test_type,
						    nullptr, false);
		REQUIRE(ret == VACCEL_EINVAL);
		ret = vaccel_resource_init_from_buf(
			&res, &buff, len, VACCEL_RESOURCE_MAX, nullptr, false);
		REQUIRE(ret == VACCEL_EINVAL);

		ret = vaccel_resource_init_from_blobs(nullptr, blobs, 1,
						      test_type);
		REQUIRE(ret == VACCEL_EINVAL);
		ret = vaccel_resource_init_from_blobs(&res, nullptr, 1,
						      test_type);
		REQUIRE(ret == VACCEL_EINVAL);
		ret = vaccel_resource_init_from_blobs(&res, blobs, 0,
						      test_type);
		REQUIRE(ret == VACCEL_EINVAL);
		ret = vaccel_resource_init_from_blobs(&res, blobs, 1,
						      VACCEL_RESOURCE_MAX);
		REQUIRE(ret == VACCEL_EINVAL);

		ret = vaccel_resource_new(nullptr, test_path, test_type);
		REQUIRE(ret == VACCEL_EINVAL);
		ret = vaccel_resource_new(&alloc_res, nullptr, test_type);
		REQUIRE(ret == VACCEL_EINVAL);
		ret = vaccel_resource_new(&alloc_res, nullptr,
					  VACCEL_RESOURCE_MAX);
		REQUIRE(ret == VACCEL_EINVAL);

		ret = vaccel_resource_multi_new(nullptr, paths, 1, test_type);
		REQUIRE(ret == VACCEL_EINVAL);
		ret = vaccel_resource_multi_new(&alloc_res, nullptr, 1,
						test_type);
		REQUIRE(ret == VACCEL_EINVAL);
		ret = vaccel_resource_multi_new(&alloc_res, paths, 0,
						test_type);
		REQUIRE(ret == VACCEL_EINVAL);
		ret = vaccel_resource_multi_new(&alloc_res, paths, 1,
						VACCEL_RESOURCE_MAX);
		REQUIRE(ret == VACCEL_EINVAL);

		ret = vaccel_resource_from_buf(nullptr, &buff, len, test_type,
					       nullptr, false);
		REQUIRE(ret == VACCEL_EINVAL);
		ret = vaccel_resource_from_buf(&alloc_res, nullptr, len,
					       test_type, nullptr, false);
		REQUIRE(ret == VACCEL_EINVAL);
		ret = vaccel_resource_from_buf(&alloc_res, &buff, 0, test_type,
					       nullptr, false);
		REQUIRE(ret == VACCEL_EINVAL);
		ret = vaccel_resource_from_buf(&alloc_res, &buff, len,
					       VACCEL_RESOURCE_MAX, nullptr,
					       false);
		REQUIRE(ret == VACCEL_EINVAL);

		ret = vaccel_resource_from_blobs(nullptr, blobs, 1, test_type);
		REQUIRE(ret == VACCEL_EINVAL);
		ret = vaccel_resource_from_blobs(&alloc_res, nullptr, 1,
						 test_type);
		REQUIRE(ret == VACCEL_EINVAL);
		ret = vaccel_resource_from_blobs(&alloc_res, blobs, 0,
						 test_type);
		REQUIRE(ret == VACCEL_EINVAL);
		ret = vaccel_resource_from_blobs(&alloc_res, blobs, 1,
						 VACCEL_RESOURCE_MAX);
		REQUIRE(ret == VACCEL_EINVAL);
	}

	SECTION("invalid paths")
	{
		ret = vaccel_resource_init_multi(&res, paths, 2, test_type);
		REQUIRE(ret == VACCEL_EINVAL);
		REQUIRE(res.paths == nullptr);
		REQUIRE(res.nr_paths == 0);

		ret = vaccel_resource_multi_new(&alloc_res, paths, 2,
						test_type);
		REQUIRE(ret == VACCEL_EINVAL);
		REQUIRE(res.paths == nullptr);
		REQUIRE(res.nr_paths == 0);
	}

	SECTION("invalid blobs")
	{
		ret = vaccel_resource_init_from_blobs(&res, blobs, 2,
						      test_type);
		REQUIRE(ret == VACCEL_EINVAL);
		REQUIRE(res.blobs == nullptr);
		REQUIRE(res.nr_blobs == 0);

		ret = vaccel_resource_from_blobs(&alloc_res, blobs, 2,
						 test_type);
		REQUIRE(ret == VACCEL_EINVAL);
		REQUIRE(res.blobs == nullptr);
		REQUIRE(res.nr_blobs == 0);
	}

	ret = vaccel_blob_release(&b);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(b.type == VACCEL_BLOB_MAX);

	free(buff);
	free(test_path);
}

// Test case for resource release failures
TEST_CASE("resource_release_fail", "[core][resource]")
{
	int ret;
	struct vaccel_resource res;
	vaccel_resource_type_t const test_type = VACCEL_RESOURCE_LIB;
	char *test_path = abs_path(BUILD_ROOT, "examples/libmytestlib.so");

	ret = vaccel_resource_init(&res, test_path, test_type);
	REQUIRE(ret == VACCEL_OK);

	struct vaccel_session sess;
	REQUIRE(vaccel_session_init(&sess, 0) == VACCEL_OK);

	ret = vaccel_resource_register(&res, &sess);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(res.blobs[0]->type == VACCEL_BLOB_FILE);
	REQUIRE_FALSE(res.blobs[0]->path_owned);
	REQUIRE_FALSE(res.blobs[0]->data_owned);

	SECTION("used resource")
	{
		ret = vaccel_resource_release(&res);
		REQUIRE(ret == VACCEL_EBUSY);
	}

	ret = vaccel_resource_unregister(&res, &sess);
	REQUIRE(ret == VACCEL_OK);

	ret = vaccel_resource_release(&res);
	REQUIRE(ret == VACCEL_OK);

	SECTION("already released resource")
	{
		ret = vaccel_resource_release(&res);
		REQUIRE(ret == VACCEL_EINVAL);
	}

	SECTION("null arguments")
	{
		ret = vaccel_resource_release(nullptr);
		REQUIRE(ret == VACCEL_EINVAL);
	}

	REQUIRE(vaccel_session_release(&sess) == VACCEL_OK);

	free(test_path);
}

// Test case for resource register/unregister to/from multiple sessions
TEST_CASE("resource_multiple_sessions", "[core][resource]")
{
	int ret;
	struct vaccel_resource res;
	char *test_path = abs_path(BUILD_ROOT, "examples/libmytestlib.so");
	vaccel_resource_type_t const test_type = VACCEL_RESOURCE_LIB;

	const size_t nr_sessions = 4;
	struct vaccel_session sessions[nr_sessions];

	ret = vaccel_resource_init(&res, test_path, test_type);
	REQUIRE(ret == VACCEL_OK);

	for (auto &sess : sessions) {
		REQUIRE(vaccel_session_init(&sess, 0) == VACCEL_OK);

		ret = vaccel_resource_register(&res, &sess);
		REQUIRE(ret == VACCEL_OK);
	}

	// Verify resource registered correctly
	REQUIRE(res.refcount == 4);
	for (auto &sess : sessions)
		REQUIRE(vaccel_session_has_resource(&sess, &res));

	for (auto &sess : sessions) {
		ret = vaccel_resource_unregister(&res, &sess);
		REQUIRE(ret == VACCEL_OK);
	}

	// Verify resource unregistered correctly
	REQUIRE(res.refcount == 0);
	for (auto &sess : sessions) {
		REQUIRE_FALSE(vaccel_session_has_resource(&sess, &res));
		REQUIRE(vaccel_session_release(&sess) == VACCEL_OK);
	}

	ret = vaccel_resource_release(&res);
	REQUIRE(ret == VACCEL_OK);

	free(test_path);
}

// Test case for resource register failures
TEST_CASE("resource_register_fail", "[core][resource]")
{
	int ret;
	struct vaccel_resource res;
	char *test_path = abs_path(BUILD_ROOT, "examples/libmytestlib.so");
	vaccel_resource_type_t const test_type = VACCEL_RESOURCE_LIB;

	struct vaccel_session sess;
	REQUIRE(vaccel_session_init(&sess, 0) == VACCEL_OK);

	ret = vaccel_resource_init(&res, test_path, test_type);
	REQUIRE(ret == VACCEL_OK);

	SECTION("null arguments")
	{
		ret = vaccel_resource_register(nullptr, &sess);
		REQUIRE(ret == VACCEL_EINVAL);

		ret = vaccel_resource_register(&res, nullptr);
		REQUIRE(ret == VACCEL_EINVAL);
	}

	ret = vaccel_resource_release(&res);
	REQUIRE(ret == VACCEL_OK);

	SECTION("released resource")
	{
		ret = vaccel_resource_register(&res, &sess);
		REQUIRE(ret == VACCEL_EINVAL);
	}

	REQUIRE(vaccel_session_release(&sess) == VACCEL_OK);

	free(test_path);
}

// Test case for resource unregister failures
TEST_CASE("resource_unregister_fail", "[core][resource]")
{
	int ret;
	struct vaccel_resource res;
	char *test_path = abs_path(BUILD_ROOT, "examples/libmytestlib.so");
	vaccel_resource_type_t const test_type = VACCEL_RESOURCE_LIB;

	struct vaccel_session sess;
	REQUIRE(vaccel_session_init(&sess, 0) == VACCEL_OK);

	ret = vaccel_resource_init(&res, test_path, test_type);
	REQUIRE(ret == VACCEL_OK);

	ret = vaccel_resource_register(&res, &sess);
	REQUIRE(ret == VACCEL_OK);

	SECTION("null arguments")
	{
		ret = vaccel_resource_unregister(nullptr, &sess);
		REQUIRE(ret == VACCEL_EINVAL);

		ret = vaccel_resource_unregister(&res, nullptr);
		REQUIRE(ret == VACCEL_EINVAL);
	}

	ret = vaccel_resource_unregister(&res, &sess);
	REQUIRE(ret == VACCEL_OK);

	ret = vaccel_resource_release(&res);
	REQUIRE(ret == VACCEL_OK);

	SECTION("released resource")
	{
		ret = vaccel_resource_register(&res, &sess);
		REQUIRE(ret == VACCEL_EINVAL);
	}

	REQUIRE(vaccel_session_release(&sess) == VACCEL_OK);

	free(test_path);
}

// Test case for finding a resource by ID failure
TEST_CASE("resource_find_by_id_fail", "[core][resource]")
{
	struct vaccel_resource *test_res = nullptr;
	vaccel_id_t const test_id = 0;

	// Attempt to find a resource by ID which fails (ID of 0 doesn't exist -
	// starts at 1)
	int const ret = vaccel_resource_get_by_id(&test_res, test_id);
	REQUIRE(ret == VACCEL_ENOENT);
}

TEST_CASE("vaccel_resource_get_by_type", "[core][resource]")
{
	struct vaccel_resource *res_ptr;
	int ret;

	/* Invalid input */
	ret = vaccel_resource_get_by_type(nullptr, VACCEL_RESOURCE_LIB);
	REQUIRE(ret == VACCEL_EINVAL);

	ret = vaccel_resource_get_by_type(&res_ptr, VACCEL_RESOURCE_MAX);
	REQUIRE(ret == VACCEL_EINVAL);

	/* Failing because there's no resource */
	ret = vaccel_resource_get_by_type(&res_ptr, VACCEL_RESOURCE_LIB);
	REQUIRE(ret == VACCEL_ENOENT);

	ret = vaccel_resource_get_by_type(&res_ptr, VACCEL_RESOURCE_MODEL);
	REQUIRE(ret == VACCEL_ENOENT);

	ret = vaccel_resource_get_by_type(&res_ptr, VACCEL_RESOURCE_DATA);
	REQUIRE(ret == VACCEL_ENOENT);

	/* Success */
	struct vaccel_resource res;
	char *lib_path = abs_path(BUILD_ROOT, "examples/libmytestlib.so");
	ret = vaccel_resource_init(&res, lib_path, VACCEL_RESOURCE_LIB);
	REQUIRE(ret == VACCEL_OK);

	ret = vaccel_resource_get_by_type(&res_ptr, VACCEL_RESOURCE_LIB);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(res_ptr == &res);

	/* Failing because of wrong type, although a resource exists */
	ret = vaccel_resource_get_by_type(&res_ptr, VACCEL_RESOURCE_MODEL);
	REQUIRE(ret == VACCEL_ENOENT);

	ret = vaccel_resource_get_by_type(&res_ptr, VACCEL_RESOURCE_DATA);
	REQUIRE(ret == VACCEL_ENOENT);

	/* Failing because the resource was released */
	ret = vaccel_resource_release(&res);
	REQUIRE(ret == VACCEL_OK);

	ret = vaccel_resource_get_by_type(&res_ptr, VACCEL_RESOURCE_LIB);
	REQUIRE(ret == VACCEL_ENOENT);

	/* Close */
	free(lib_path);
}

TEST_CASE("vaccel_resource_get_all_by_type", "[core][resource]")
{
	struct vaccel_resource **res_ptr;
	size_t nr_found;
	int ret;

	/* Invalid input */
	ret = vaccel_resource_get_all_by_type(nullptr, &nr_found,
					      VACCEL_RESOURCE_LIB);
	REQUIRE(ret == VACCEL_EINVAL);

	ret = vaccel_resource_get_all_by_type(&res_ptr, nullptr,
					      VACCEL_RESOURCE_LIB);
	REQUIRE(ret == VACCEL_EINVAL);

	ret = vaccel_resource_get_all_by_type(&res_ptr, &nr_found,
					      VACCEL_RESOURCE_MAX);
	REQUIRE(ret == VACCEL_EINVAL);

	/* Failing because there's no resource */
	ret = vaccel_resource_get_all_by_type(&res_ptr, &nr_found,
					      VACCEL_RESOURCE_LIB);
	REQUIRE(ret == VACCEL_ENOENT);
	REQUIRE(nr_found == 0);

	ret = vaccel_resource_get_all_by_type(&res_ptr, &nr_found,
					      VACCEL_RESOURCE_DATA);
	REQUIRE(ret == VACCEL_ENOENT);
	REQUIRE(nr_found == 0);

	ret = vaccel_resource_get_all_by_type(&res_ptr, &nr_found,
					      VACCEL_RESOURCE_MODEL);
	REQUIRE(ret == VACCEL_ENOENT);
	REQUIRE(nr_found == 0);

	/* Success */
	struct vaccel_resource res;
	char *lib_path = abs_path(BUILD_ROOT, "examples/libmytestlib.so");
	ret = vaccel_resource_init(&res, lib_path, VACCEL_RESOURCE_LIB);
	REQUIRE(ret == VACCEL_OK);

	ret = vaccel_resource_get_all_by_type(&res_ptr, &nr_found,
					      VACCEL_RESOURCE_LIB);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(nr_found == 1);
	REQUIRE(res_ptr[0] == &res);

	free(res_ptr);

	/* Success with more than 1 resource */
	struct vaccel_resource res2;
	ret = vaccel_resource_init(&res2, lib_path, VACCEL_RESOURCE_LIB);
	REQUIRE(ret == VACCEL_OK);

	ret = vaccel_resource_get_all_by_type(&res_ptr, &nr_found,
					      VACCEL_RESOURCE_LIB);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(nr_found == 2);
	REQUIRE(res_ptr[0] == &res);
	REQUIRE(res_ptr[1] == &res2);

	free(res_ptr);

	/* Delete the second resource */
	ret = vaccel_resource_release(&res2);
	REQUIRE(ret == VACCEL_OK);

	/* Failing because of wrong type, although a resource exists */
	ret = vaccel_resource_get_all_by_type(&res_ptr, &nr_found,
					      VACCEL_RESOURCE_MODEL);
	REQUIRE(ret == VACCEL_ENOENT);
	REQUIRE(nr_found == 0);

	ret = vaccel_resource_get_all_by_type(&res_ptr, &nr_found,
					      VACCEL_RESOURCE_DATA);
	REQUIRE(ret == VACCEL_ENOENT);
	REQUIRE(nr_found == 0);

	/* Failing because the resource was released */
	ret = vaccel_resource_release(&res);
	REQUIRE(ret == VACCEL_OK);

	ret = vaccel_resource_get_all_by_type(&res_ptr, &nr_found,
					      VACCEL_RESOURCE_LIB);
	REQUIRE(ret == VACCEL_ENOENT);
	REQUIRE(nr_found == 0);

	/* Close */
	free(lib_path);
}

TEST_CASE("memory_only_resource", "[core][resource]")
{
	int ret;
	char *file = abs_path(BUILD_ROOT, "examples/libmytestlib.so");
	const size_t nr_resources = 2;
	struct vaccel_resource *resources[nr_resources];
	struct vaccel_session sess;

	/* Session init */
	REQUIRE(vaccel_session_init(&sess, 0) == VACCEL_OK);

	/* Read blob */
	size_t len;
	unsigned char *buff;
	ret = fs_file_read(file, (void **)&buff, &len);
	REQUIRE(ret == VACCEL_OK);

	/* Resource init from buffer */
	struct vaccel_resource res;
	ret = vaccel_resource_init_from_buf(&res, buff, len,
					    VACCEL_RESOURCE_LIB, nullptr, true);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(res.id > 0);
	REQUIRE(res.remote_id == -1);
	REQUIRE(res.type == VACCEL_RESOURCE_LIB);
	REQUIRE(res.path_type == VACCEL_PATH_LOCAL_FILE);
	REQUIRE(res.nr_paths == 0);
	REQUIRE(res.blobs);
	REQUIRE(res.blobs[0]);
	REQUIRE(res.blobs[0]->type == VACCEL_BLOB_BUFFER);
	REQUIRE(res.blobs[0]->data);
	REQUIRE(res.blobs[0]->data == buff);
	REQUIRE(res.blobs[0]->size == len);
	REQUIRE(res.blobs[0]->path == nullptr);
	REQUIRE_FALSE(res.blobs[0]->data_owned);
	REQUIRE_FALSE(res.blobs[0]->path_owned);
	REQUIRE(res.rundir == nullptr);
	REQUIRE(res.paths == nullptr);
	REQUIRE(res.nr_paths == 0);
	REQUIRE_FALSE(list_empty(&res.entry));
	REQUIRE(res.refcount == 0);
	resources[0] = &res;

	/* Resource new from buffer */
	struct vaccel_resource *alloc_res;
	ret = vaccel_resource_from_buf(&alloc_res, buff, len,
				       VACCEL_RESOURCE_LIB, nullptr, true);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(alloc_res->id == res.id + 1);
	REQUIRE(alloc_res->remote_id == -1);
	REQUIRE(alloc_res->type == VACCEL_RESOURCE_LIB);
	REQUIRE(alloc_res->path_type == VACCEL_PATH_LOCAL_FILE);
	REQUIRE(alloc_res->nr_paths == 0);
	REQUIRE(alloc_res->blobs);
	REQUIRE(alloc_res->blobs[0]);
	REQUIRE(alloc_res->blobs[0]->type == VACCEL_BLOB_BUFFER);
	REQUIRE(alloc_res->blobs[0]->data);
	REQUIRE(alloc_res->blobs[0]->data == buff);
	REQUIRE(alloc_res->blobs[0]->size == len);
	REQUIRE(alloc_res->blobs[0]->path == nullptr);
	REQUIRE_FALSE(alloc_res->blobs[0]->data_owned);
	REQUIRE_FALSE(alloc_res->blobs[0]->path_owned);
	REQUIRE(alloc_res->rundir == nullptr);
	REQUIRE(alloc_res->paths == nullptr);
	REQUIRE(alloc_res->nr_paths == 0);
	REQUIRE_FALSE(list_empty(&alloc_res->entry));
	REQUIRE(alloc_res->refcount == 0);
	resources[1] = alloc_res;

	for (auto &resource : resources) {
		/* Register resources */
		ret = vaccel_resource_register(resource, &sess);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(resource->refcount == 1);

		/* Requirements */
		REQUIRE(resource->type == VACCEL_RESOURCE_LIB);
		REQUIRE(resource->path_type == VACCEL_PATH_LOCAL_FILE);
		REQUIRE(resource->nr_paths == 0);
		REQUIRE(resource->blobs);
		REQUIRE(resource->blobs[0]);
		REQUIRE(resource->blobs[0]->type == VACCEL_BLOB_BUFFER);
		REQUIRE(resource->blobs[0]->data);
		REQUIRE(resource->blobs[0]->data == buff);
		REQUIRE(resource->blobs[0]->size == len);
		REQUIRE(resource->blobs[0]->path == nullptr);
		REQUIRE_FALSE(resource->blobs[0]->data_owned);
		REQUIRE_FALSE(resource->blobs[0]->path_owned);
		REQUIRE(resource->rundir == nullptr);
		REQUIRE(resource->paths == nullptr);
		REQUIRE(resource->nr_paths == 0);

		/* Get by id */
		struct vaccel_resource *found_by_id;
		ret = vaccel_resource_get_by_id(&found_by_id, resource->id);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(found_by_id == resource);

		/* Has resource */
		REQUIRE(vaccel_session_has_resource(&sess, resource));

		/* Unregister resources */
		ret = vaccel_resource_unregister(resource, &sess);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE_FALSE(vaccel_session_has_resource(&sess, resource));
		REQUIRE(resource->refcount == 0);
	}

	/* Free resource */
	ret = vaccel_resource_delete(alloc_res);
	REQUIRE(ret == VACCEL_OK);

	/* Release resource */
	ret = vaccel_resource_release(&res);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(res.rundir == nullptr);
	REQUIRE(res.blobs == nullptr);
	REQUIRE(res.nr_blobs == 0);
	REQUIRE(res.paths == nullptr);
	REQUIRE(res.nr_paths == 0);
	REQUIRE(res.id <= 0);

	/* Release session */
	REQUIRE(vaccel_session_release(&sess) == VACCEL_OK);

	free(buff);
	free(file);
}

TEST_CASE("memory_only_resource_virtio", "[core][resource]")
{
	int ret;
	char *file = abs_path(BUILD_ROOT, "examples/libmytestlib.so");
	struct vaccel_session vsess;

	RESET_FAKE(plugin_virtio);
	plugin_virtio_fake.custom_fake = mock_virtio_plugin_virtio;

	/* Session init */
	REQUIRE(vaccel_session_init(&vsess, VACCEL_PLUGIN_REMOTE) == VACCEL_OK);

	/* Read blob */
	size_t len;
	unsigned char *buff;
	ret = fs_file_read(file, (void **)&buff, &len);
	REQUIRE(ret == VACCEL_OK);

	/* Resource init from buffer */
	struct vaccel_resource res;
	ret = vaccel_resource_init_from_buf(&res, buff, len,
					    VACCEL_RESOURCE_LIB, nullptr, true);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(res.id > 0);
	REQUIRE(res.remote_id == -1);
	REQUIRE(res.type == VACCEL_RESOURCE_LIB);
	REQUIRE(res.path_type == VACCEL_PATH_LOCAL_FILE);
	REQUIRE(res.nr_paths == 0);
	REQUIRE(res.blobs);
	REQUIRE(res.blobs[0]);
	REQUIRE(res.blobs[0]->type == VACCEL_BLOB_BUFFER);
	REQUIRE(res.blobs[0]->data);
	REQUIRE(res.blobs[0]->data == buff);
	REQUIRE(res.blobs[0]->size == len);
	REQUIRE(res.blobs[0]->path == nullptr);
	REQUIRE_FALSE(res.blobs[0]->data_owned);
	REQUIRE_FALSE(res.blobs[0]->path_owned);
	REQUIRE(res.rundir == nullptr);
	REQUIRE(res.paths == nullptr);
	REQUIRE(res.nr_paths == 0);
	REQUIRE_FALSE(list_empty(&res.entry));
	REQUIRE(res.refcount == 0);

	/* Register resource */
	ret = vaccel_resource_register(&res, &vsess);
	REQUIRE(ret == VACCEL_OK);

	/* Requirements */
	REQUIRE(res.type == VACCEL_RESOURCE_LIB);
	REQUIRE(res.path_type == VACCEL_PATH_LOCAL_FILE);
	REQUIRE(res.nr_paths == 0);
	REQUIRE(res.blobs);
	REQUIRE(res.blobs[0]);
	REQUIRE(res.blobs[0]->type == VACCEL_BLOB_BUFFER);
	REQUIRE(res.blobs[0]->data);
	REQUIRE(res.blobs[0]->data == buff);
	REQUIRE(res.blobs[0]->size == len);
	REQUIRE(res.blobs[0]->path == nullptr);
	REQUIRE_FALSE(res.blobs[0]->data_owned);
	REQUIRE_FALSE(res.blobs[0]->path_owned);
	REQUIRE(res.rundir == nullptr);
	REQUIRE(res.paths == nullptr);
	REQUIRE(res.nr_paths == 0);

	/* Unregister resource */
	ret = vaccel_resource_unregister(&res, &vsess);
	REQUIRE(ret == VACCEL_OK);

	/* Release resource */
	ret = vaccel_resource_release(&res);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(res.rundir == nullptr);
	REQUIRE(res.blobs == nullptr);
	REQUIRE(res.nr_blobs == 0);
	REQUIRE(res.paths == nullptr);
	REQUIRE(res.nr_paths == 0);
	REQUIRE(res.id <= 0);

	/* Release session */
	REQUIRE(vaccel_session_release(&vsess) == VACCEL_OK);

	free(buff);
	free(file);
}

// Test case for resource component not bootstrapped
TEST_CASE("resources_not_bootstrapped", "[core][resource]")
{
	int ret;
	struct vaccel_resource res;
	vaccel_resource_type_t const test_type = VACCEL_RESOURCE_LIB;

	// cleanup here so resources are not bootstrapped
	ret = resources_cleanup();
	REQUIRE(ret == VACCEL_OK);

	ret = vaccel_resource_init(&res, nullptr, test_type);
	REQUIRE(ret == VACCEL_EPERM);

	ret = vaccel_resource_init_multi(&res, nullptr, 0, test_type);
	REQUIRE(ret == VACCEL_EPERM);

	ret = vaccel_resource_init_from_buf(&res, nullptr, 0, test_type,
					    nullptr, false);
	REQUIRE(ret == VACCEL_EPERM);

	ret = vaccel_resource_init_from_blobs(&res, nullptr, 0, test_type);
	REQUIRE(ret == VACCEL_EPERM);

	ret = vaccel_resource_register(&res, nullptr);
	REQUIRE(ret == VACCEL_EPERM);

	ret = vaccel_resource_unregister(&res, nullptr);
	REQUIRE(ret == VACCEL_EPERM);

	ret = vaccel_resource_get_by_id(nullptr, 0);
	REQUIRE(ret == VACCEL_EPERM);

	ret = vaccel_resource_release(&res);
	REQUIRE(ret == VACCEL_EPERM);

	ret = resource_create_rundir(nullptr);
	REQUIRE(ret == VACCEL_EINVAL);

	// bootstrap again so the rest of the tests run correctly
	ret = resources_bootstrap();
	REQUIRE(ret == VACCEL_OK);
}
