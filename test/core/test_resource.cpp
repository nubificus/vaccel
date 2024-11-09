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
 * 12) vaccel_resource_register()
 * 13) vaccel_resource_unregister()
 * 14) vaccel_resource_release()
 * 15) vaccel_resource_directory()
 * 16) vaccel_session_has_resource()
 *
 *
 */

#include <catch.hpp>
#include <fff.h>
#include <mock_virtio.hpp>
#include <utils.hpp>
#include <vaccel.h>

DEFINE_FFF_GLOBALS;

extern "C" {
FAKE_VALUE_FUNC(struct vaccel_plugin *, get_virtio_plugin);
FAKE_VALUE_FUNC(int, net_nocurl_file_download, const char *, const char *);
}

#define DOWNLOAD_FILE "examples/models/torch/cnn_trace.pt"

static auto mock_net_nocurl_file_download(const char *path,
					  const char *download_path) -> int
{
	(void)path;

	FILE *fp = fopen(download_path, "wb");
	if (fp == nullptr) {
		vaccel_error("Could not open file %s: %s", download_path,
			     strerror(errno));
		return VACCEL_EIO;
	}

	char *file_to_copy = abs_path(SOURCE_ROOT, DOWNLOAD_FILE);

	unsigned char *buf;
	size_t len;
	int ret = fs_file_read(file_to_copy, (void **)&buf, &len);
	if (ret != 0) {
		vaccel_error("Could not read file %s", file_to_copy);
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
	REQUIRE(res.id == 1);
	REQUIRE(res.remote_id == -1);
	REQUIRE(res.files == nullptr);
	REQUIRE(res.nr_files == 0);
	REQUIRE(res.type == VACCEL_RESOURCE_DATA);
	REQUIRE(res.path_type == VACCEL_PATH_DIR);
	REQUIRE(res.nr_paths == 1);
	REQUIRE(res.rundir == nullptr);
	REQUIRE_FALSE(list_empty(&res.entry));
	REQUIRE(res.refcount == 0);
	resources[0] = &res;

	/* Resource new from dir path */
	struct vaccel_resource *alloc_res;
	ret = vaccel_resource_new(&alloc_res, dir_path, VACCEL_RESOURCE_DATA);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(alloc_res->id == 2);
	REQUIRE(alloc_res->remote_id == -1);
	REQUIRE(alloc_res->files == nullptr);
	REQUIRE(alloc_res->nr_files == 0);
	REQUIRE(alloc_res->type == VACCEL_RESOURCE_DATA);
	REQUIRE(alloc_res->path_type == VACCEL_PATH_DIR);
	REQUIRE(alloc_res->nr_paths == 1);
	REQUIRE(alloc_res->rundir == nullptr);
	REQUIRE_FALSE(list_empty(&alloc_res->entry));
	REQUIRE(alloc_res->refcount == 0);
	resources[1] = alloc_res;

	/* Session init */
	struct vaccel_session sess;
	ret = vaccel_session_init(&sess, 0);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(sess.id == 1);

	/* Register resources */
	for (auto &resource : resources) {
		ret = vaccel_resource_register(resource, &sess);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(resource->nr_files ==
			(size_t)fs_dir_process_files(dir_path, nullptr));
		REQUIRE(resource->files);
		REQUIRE(resource->refcount == 1);
	}

	const char *dir_files[4] = { "keras_metadata.pb", "saved_model.pb",
				     "variables.data-00000-of-00001",
				     "variables.index" };
	for (size_t i = 0; i < nr_resources; i++) {
		/* Check files */
		for (size_t j = 0; j < resources[i]->nr_files; j++) {
			REQUIRE(resources[i]->files[j]);
			REQUIRE(resources[i]->files[j]->path);
			REQUIRE(fs_path_is_file(resources[i]->files[j]->path));

			char filename[NAME_MAX];
			path_file_name(resources[i]->files[j]->path, filename,
				       NAME_MAX, nullptr);
			REQUIRE(string_in_list(filename, dir_files, 4));
		}

		/* Get resource directory */
		char dir_resp[PATH_MAX] = { '\0' };
		ret = vaccel_resource_directory(resources[i], dir_resp,
						sizeof(dir_resp), nullptr);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(strcmp(dir_resp, dir_path) == 0);

		/* Get resource directory with alloc */
		char *dir_resp_alloc;
		ret = vaccel_resource_directory(resources[i], nullptr, 0,
						&dir_resp_alloc);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(strcmp(dir_resp_alloc, dir_path) == 0);
		free(dir_resp_alloc);

		/* Get by id */
		struct vaccel_resource *found_by_id;
		ret = vaccel_resource_get_by_id(&found_by_id,
						(vaccel_id_t)i + 1);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(found_by_id == resources[i]);

		/* Has resource */
		REQUIRE(vaccel_session_has_resource(&sess, resources[i]));
	}

	SECTION("virtio session")
	{
		RESET_FAKE(get_virtio_plugin);

		get_virtio_plugin_fake.custom_fake =
			mock_virtio_get_virtio_plugin;

		/* Session init */
		struct vaccel_session vsess;
		ret = vaccel_session_init(&vsess, 0 | VACCEL_REMOTE);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(vsess.id == 2);
		REQUIRE(vsess.remote_id == 1);

		/* Register resource */
		ret = vaccel_resource_register(&res, &vsess);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(res.nr_files ==
			(size_t)fs_dir_process_files(dir_path, nullptr));
		REQUIRE(res.files);
		REQUIRE(res.refcount == 2);
		REQUIRE(res.id == 1);
		REQUIRE(res.remote_id == 1);

		/* Check files */
		const char *dir_files[4] = { "keras_metadata.pb",
					     "saved_model.pb",
					     "variables.data-00000-of-00001",
					     "variables.index" };
		for (size_t i = 0; i < res.nr_files; i++) {
			REQUIRE(res.files[i]);
			REQUIRE(res.files[i]->path);
			REQUIRE(res.files[i]->data);
			REQUIRE(string_in_list(res.files[i]->name, dir_files,
					       4));

			size_t len;
			unsigned char *buf;
			ret = fs_file_read(res.files[i]->path, (void **)&buf,
					   &len);
			REQUIRE(ret == VACCEL_OK);

			REQUIRE(res.files[i]->size == len);
			for (size_t j = 0; j < len; j++) {
				REQUIRE(res.files[i]->data[j] == buf[j]);
			}
			free(buf);
		}

		/* Get by id */
		struct vaccel_resource *found_by_id;
		ret = vaccel_resource_get_by_id(&found_by_id, 1);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(found_by_id == &res);

		/* Has resource */
		REQUIRE(vaccel_session_has_resource(&vsess, &res));

		/* Unregister resource */
		ret = vaccel_resource_unregister(&res, &vsess);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE_FALSE(vaccel_session_has_resource(&vsess, &res));

		/* Release session */
		ret = vaccel_session_release(&vsess);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(vsess.id <= 0);

		REQUIRE(get_virtio_plugin_fake.call_count == 4);
	}

	/* Unregister resources */
	for (auto &resource : resources) {
		ret = vaccel_resource_unregister(resource, &sess);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE_FALSE(vaccel_session_has_resource(&sess, resource));
	}

	/* Release session */
	ret = vaccel_session_release(&sess);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(sess.id <= 0);

	/* Free resource */
	ret = vaccel_resource_delete(alloc_res);
	REQUIRE(ret == VACCEL_OK);

	/* Release resource */
	ret = vaccel_resource_release(&res);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(res.rundir == nullptr);
	REQUIRE(res.files == nullptr);
	REQUIRE(res.nr_files == 0);
	REQUIRE(res.paths == nullptr);
	REQUIRE(res.nr_paths == 0);
	REQUIRE(res.id <= 0);

	free(dir_path);
}

// Test case for resource from file paths operations
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

	/* Resource init from file paths */
	struct vaccel_resource res;
	ret = vaccel_resource_init_multi(&res, paths,
					 sizeof(paths) / sizeof(char *),
					 VACCEL_RESOURCE_DATA);
	REQUIRE(ret == VACCEL_OK);
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
	REQUIRE(res.files == nullptr);
	REQUIRE(res.nr_files == 0);
	REQUIRE(res.rundir == nullptr);
	REQUIRE_FALSE(list_empty(&res.entry));
	REQUIRE(res.refcount == 0);
	resources[0] = &res;

	/* Resource new from file paths */
	struct vaccel_resource *alloc_res;
	ret = vaccel_resource_multi_new(&alloc_res, paths,
					sizeof(paths) / sizeof(char *),
					VACCEL_RESOURCE_DATA);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(alloc_res->id == 2);
	REQUIRE(alloc_res->remote_id == -1);
	REQUIRE(alloc_res->type == VACCEL_RESOURCE_DATA);
	REQUIRE(alloc_res->path_type == VACCEL_PATH_LOCAL);
	REQUIRE(alloc_res->nr_paths == 2);
	REQUIRE(alloc_res->paths);
	for (size_t i = 0; i < alloc_res->nr_paths; i++) {
		REQUIRE(alloc_res->paths[i]);
		REQUIRE(string_in_list(alloc_res->paths[i], paths, 2));
	}
	REQUIRE(alloc_res->files == nullptr);
	REQUIRE(alloc_res->nr_files == 0);
	REQUIRE(alloc_res->rundir == nullptr);
	REQUIRE_FALSE(list_empty(&alloc_res->entry));
	REQUIRE(alloc_res->refcount == 0);
	resources[1] = alloc_res;

	/* Session init */
	struct vaccel_session sess;
	ret = vaccel_session_init(&sess, 0);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(sess.id == 1);

	/* Register resources */
	for (auto &resource : resources) {
		ret = vaccel_resource_register(resource, &sess);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(resource->nr_files == 2);
		REQUIRE(resource->files);
		REQUIRE(resource->refcount == 1);
	}

	for (size_t i = 0; i < nr_resources; i++) {
		/* Check files */
		for (size_t j = 0; j < resources[i]->nr_files; j++) {
			REQUIRE(resources[i]->files[j]);
			REQUIRE(resources[i]->files[j]->path);
			REQUIRE(fs_path_is_file(resources[i]->files[j]->path));
			REQUIRE(string_in_list(resources[i]->files[j]->path,
					       paths, 2));
		}

		/* Get by id */
		struct vaccel_resource *found_by_id;
		ret = vaccel_resource_get_by_id(&found_by_id,
						(vaccel_id_t)i + 1);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(found_by_id == resources[i]);

		/* Has resource */
		REQUIRE(vaccel_session_has_resource(&sess, resources[i]));
	}

	SECTION("virtio session")
	{
		RESET_FAKE(get_virtio_plugin);

		get_virtio_plugin_fake.custom_fake =
			mock_virtio_get_virtio_plugin;

		/* Session init */
		struct vaccel_session vsess;
		ret = vaccel_session_init(&vsess, 0 | VACCEL_REMOTE);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(vsess.id == 2);
		REQUIRE(vsess.remote_id == 1);

		/* Register resource */
		ret = vaccel_resource_register(&res, &vsess);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(res.nr_files == 2);
		REQUIRE(res.files);
		REQUIRE(res.refcount == 2);
		REQUIRE(res.id == 1);
		REQUIRE(res.remote_id == 1);

		/* Check files */
		for (size_t i = 0; i < res.nr_files; i++) {
			REQUIRE(res.files[i]);
			REQUIRE(res.files[i]->path);
			REQUIRE(res.files[i]->data);
			REQUIRE(string_in_list(res.files[i]->path, paths, 2));

			size_t len;
			unsigned char *buf;
			ret = fs_file_read(res.files[i]->path, (void **)&buf,
					   &len);
			REQUIRE(ret == VACCEL_OK);

			REQUIRE(res.files[i]->size == len);
			for (size_t j = 0; j < len; j++) {
				REQUIRE(res.files[i]->data[j] == buf[j]);
			}
			free(buf);
		}

		/* Get by id */
		struct vaccel_resource *found_by_id;
		ret = vaccel_resource_get_by_id(&found_by_id, 1);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(found_by_id == &res);

		/* Has resource */
		REQUIRE(vaccel_session_has_resource(&vsess, &res));

		/* Unregister resource */
		ret = vaccel_resource_unregister(&res, &vsess);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE_FALSE(vaccel_session_has_resource(&vsess, &res));

		/* Release session */
		ret = vaccel_session_release(&vsess);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(vsess.id <= 0);

		REQUIRE(get_virtio_plugin_fake.call_count == 4);
	}

	/* Unregister resources */
	for (auto &resource : resources) {
		ret = vaccel_resource_unregister(resource, &sess);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE_FALSE(vaccel_session_has_resource(&sess, resource));
	}

	/* Release session */
	ret = vaccel_session_release(&sess);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(sess.id <= 0);

	/* Free resource */
	ret = vaccel_resource_delete(alloc_res);
	REQUIRE(ret == VACCEL_OK);

	/* Release resource */
	ret = vaccel_resource_release(&res);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(res.rundir == nullptr);
	REQUIRE(res.files == nullptr);
	REQUIRE(res.nr_files == 0);
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
	REQUIRE(res.id == 1);
	REQUIRE(res.remote_id == -1);
	REQUIRE(res.type == VACCEL_RESOURCE_DATA);
	REQUIRE(res.path_type == VACCEL_PATH_REMOTE);
	REQUIRE(res.nr_paths == 1);
	REQUIRE(res.paths);
	REQUIRE(res.paths[0]);
	REQUIRE(res.files == nullptr);
	REQUIRE(res.nr_files == 0);
	REQUIRE(res.rundir == nullptr);
	REQUIRE_FALSE(list_empty(&res.entry));
	REQUIRE(res.refcount == 0);
	resources[0] = &res;

	/* Resource init from url */
	struct vaccel_resource *alloc_res;
	ret = vaccel_resource_new(&alloc_res, url, VACCEL_RESOURCE_DATA);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(alloc_res->id == 2);
	REQUIRE(alloc_res->remote_id == -1);
	REQUIRE(alloc_res->type == VACCEL_RESOURCE_DATA);
	REQUIRE(alloc_res->path_type == VACCEL_PATH_REMOTE);
	REQUIRE(alloc_res->nr_paths == 1);
	REQUIRE(alloc_res->paths);
	REQUIRE(alloc_res->paths[0]);
	REQUIRE(alloc_res->files == nullptr);
	REQUIRE(alloc_res->nr_files == 0);
	REQUIRE(alloc_res->rundir == nullptr);
	REQUIRE_FALSE(list_empty(&alloc_res->entry));
	REQUIRE(alloc_res->refcount == 0);
	resources[1] = alloc_res;

	/* Session init */
	struct vaccel_session sess;
	ret = vaccel_session_init(&sess, 0);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(sess.id == 1);

	SECTION("virtio session")
	{
		RESET_FAKE(get_virtio_plugin);

		get_virtio_plugin_fake.custom_fake =
			mock_virtio_get_virtio_plugin;

		/* Session init */
		struct vaccel_session vsess;
		ret = vaccel_session_init(&vsess, 0 | VACCEL_REMOTE);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(vsess.id == 2);
		REQUIRE(vsess.remote_id == 1);

		/* Register resource */
		ret = vaccel_resource_register(&res, &vsess);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(res.nr_files == 0);
		REQUIRE_FALSE(res.files);
		REQUIRE(res.refcount == 1);
		REQUIRE(res.id == 1);
		REQUIRE(res.remote_id == 1);

		/* Get by id */
		struct vaccel_resource *found_by_id;
		ret = vaccel_resource_get_by_id(&found_by_id, 1);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(found_by_id == &res);

		/* Has resource */
		REQUIRE(vaccel_session_has_resource(&vsess, &res));

		/* Unregister resource */
		ret = vaccel_resource_unregister(&res, &vsess);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE_FALSE(vaccel_session_has_resource(&vsess, &res));

		/* Release session */
		ret = vaccel_session_release(&vsess);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(vsess.id <= 0);

		REQUIRE(get_virtio_plugin_fake.call_count == 4);
	}

	/* Register resources */
	for (auto &resource : resources) {
		ret = vaccel_resource_register(resource, &sess);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(resource->rundir);
		REQUIRE(resource->nr_files == 1);
		REQUIRE(resource->files);
		REQUIRE(resource->files[0]);
		REQUIRE(resource->files[0]->path);
		REQUIRE(resource->refcount == 1);
	}

	char *file = abs_path(SOURCE_ROOT, DOWNLOAD_FILE);
	size_t len1;
	unsigned char *buf1;
	ret = fs_file_read(file, (void **)&buf1, &len1);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(buf1);
	REQUIRE(len1 > 0);
	for (size_t i = 0; i < nr_resources; i++) {
		/* Check content */
		size_t len2;
		unsigned char *buf2;
		ret = fs_file_read(resources[i]->files[0]->path, (void **)&buf2,
				   &len2);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(buf2);
		REQUIRE(len2 > 0);
		free(buf2);

		/* Get by id */
		struct vaccel_resource *found_by_id;
		ret = vaccel_resource_get_by_id(&found_by_id,
						(vaccel_id_t)i + 1);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(found_by_id == resources[i]);

		/* Has resource */
		REQUIRE(vaccel_session_has_resource(&sess, resources[i]));
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
	ret = vaccel_session_release(&sess);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(sess.id <= 0);

	/* Free resource */
	ret = vaccel_resource_delete(alloc_res);
	REQUIRE(ret == VACCEL_OK);

	/* Release resource */
	ret = vaccel_resource_release(&res);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(res.rundir == nullptr);
	REQUIRE(res.files == nullptr);
	REQUIRE(res.nr_files == 0);
	REQUIRE(res.paths == nullptr);
	REQUIRE(res.nr_paths == 0);
	REQUIRE(res.id <= 0);
}

// Test case for resource from buffer operations
TEST_CASE("resource_from_buffer", "[core][resource]")
{
	int ret;
	char *file = abs_path(BUILD_ROOT, "examples/libmytestlib.so");

	struct vaccel_session sess;

	/* Session init */
	ret = vaccel_session_init(&sess, 0);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(sess.id == 1);

	/* Read file */
	size_t len;
	unsigned char *buff;
	ret = fs_file_read(file, (void **)&buff, &len);
	REQUIRE(ret == VACCEL_OK);

	/* Resource init from buffer */
	struct vaccel_resource res;
	ret = vaccel_resource_init_from_buf(&res, buff, len,
					    VACCEL_RESOURCE_LIB, "lib.so");
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(res.id == 1);
	REQUIRE(res.remote_id == -1);
	REQUIRE(res.type == VACCEL_RESOURCE_LIB);
	REQUIRE(res.path_type == VACCEL_PATH_LOCAL);
	REQUIRE(res.nr_paths == 0);
	REQUIRE(res.files);
	REQUIRE(res.files[0]);
	REQUIRE(res.rundir);
	REQUIRE(res.paths == nullptr);
	REQUIRE(res.nr_paths == 0);
	REQUIRE_FALSE(list_empty(&res.entry));
	REQUIRE(res.refcount == 0);

	/* Resource new from buffer */
	struct vaccel_resource *alloc_res;
	ret = vaccel_resource_from_buf(&alloc_res, buff, len,
				       VACCEL_RESOURCE_LIB, "lib.so");
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(alloc_res->id == 2);
	REQUIRE(alloc_res->remote_id == -1);
	REQUIRE(alloc_res->type == VACCEL_RESOURCE_LIB);
	REQUIRE(alloc_res->path_type == VACCEL_PATH_LOCAL);
	REQUIRE(alloc_res->nr_paths == 0);
	REQUIRE(alloc_res->files);
	REQUIRE(alloc_res->files[0]);
	REQUIRE(alloc_res->rundir);
	REQUIRE(alloc_res->paths == nullptr);
	REQUIRE(alloc_res->nr_paths == 0);
	REQUIRE_FALSE(list_empty(&alloc_res->entry));
	REQUIRE(alloc_res->refcount == 0);

	/* Register resources */
	ret = vaccel_resource_register(&res, &sess);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(res.refcount == 1);

	ret = vaccel_resource_register(alloc_res, &sess);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(alloc_res->refcount == 1);

	/* Get by id */
	struct vaccel_resource *found_by_id;
	ret = vaccel_resource_get_by_id(&found_by_id, 1);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(found_by_id == &res);

	ret = vaccel_resource_get_by_id(&found_by_id, 2);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(found_by_id == alloc_res);

	/* Has resource */
	REQUIRE(vaccel_session_has_resource(&sess, &res));
	REQUIRE(vaccel_session_has_resource(&sess, alloc_res));

	/* Unregister resources */
	ret = vaccel_resource_unregister(&res, &sess);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE_FALSE(vaccel_session_has_resource(&sess, &res));
	REQUIRE(res.refcount == 0);

	ret = vaccel_resource_unregister(alloc_res, &sess);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE_FALSE(vaccel_session_has_resource(&sess, alloc_res));
	REQUIRE(alloc_res->refcount == 0);

	/* Free resource */
	ret = vaccel_resource_delete(alloc_res);
	REQUIRE(ret == VACCEL_OK);

	/* Release resource */
	ret = vaccel_resource_release(&res);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(res.rundir == nullptr);
	REQUIRE(res.files == nullptr);
	REQUIRE(res.nr_files == 0);
	REQUIRE(res.paths == nullptr);
	REQUIRE(res.nr_paths == 0);
	REQUIRE(res.id <= 0);

	/* Release session */
	ret = vaccel_session_release(&sess);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(sess.id <= 0);

	free(buff);
	free(file);
}

// Test case for resource from vaccel files operations
TEST_CASE("resource_from_files", "[core][resource]")
{
	int ret;
	char *path1 = abs_path(
		SOURCE_ROOT,
		"examples/models/tf/lstm2/variables/variables.data-00000-of-00001");
	char *path2 =
		abs_path(SOURCE_ROOT,
			 "examples/models/tf/lstm2/variables/variables.index");

	struct vaccel_file f1;
	struct vaccel_file f2;

	/* Init files */
	ret = vaccel_file_init(&f1, path1);
	REQUIRE(ret == VACCEL_OK);

	ret = vaccel_file_init(&f2, path2);
	REQUIRE(ret == VACCEL_OK);

	/* Read files */
	ret = vaccel_file_read(&f1);
	REQUIRE(ret == VACCEL_OK);

	ret = vaccel_file_read(&f2);
	REQUIRE(ret == VACCEL_OK);

	const struct vaccel_file *vaccel_files[2] = { &f1, &f2 };

	/* Resource init from files */
	struct vaccel_resource res;
	ret = vaccel_resource_init_from_files(&res, vaccel_files, 2,
					      VACCEL_RESOURCE_DATA);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(res.id == 1);
	REQUIRE(res.remote_id == -1);
	REQUIRE(res.type == VACCEL_RESOURCE_DATA);
	REQUIRE(res.path_type == VACCEL_PATH_LOCAL);
	REQUIRE(res.paths == nullptr);
	REQUIRE(res.nr_paths == 0);
	REQUIRE(res.rundir);
	REQUIRE(res.files);
	REQUIRE(res.nr_files == 2);
	REQUIRE(res.files[0]);
	REQUIRE(res.files[1]);
	REQUIRE_FALSE(list_empty(&res.entry));
	REQUIRE(res.refcount == 0);

	/* Resource new from files */
	struct vaccel_resource *alloc_res;
	ret = vaccel_resource_from_files(&alloc_res, vaccel_files, 2,
					 VACCEL_RESOURCE_DATA);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(alloc_res->id == 2);
	REQUIRE(alloc_res->remote_id == -1);
	REQUIRE(alloc_res->type == VACCEL_RESOURCE_DATA);
	REQUIRE(alloc_res->path_type == VACCEL_PATH_LOCAL);
	REQUIRE(alloc_res->paths == nullptr);
	REQUIRE(alloc_res->nr_paths == 0);
	REQUIRE(alloc_res->rundir);
	REQUIRE(alloc_res->files);
	REQUIRE(alloc_res->nr_files == 2);
	REQUIRE(alloc_res->files[0]);
	REQUIRE(alloc_res->files[1]);
	REQUIRE_FALSE(list_empty(&alloc_res->entry));
	REQUIRE(alloc_res->refcount == 0);

	/* Session init */
	struct vaccel_session sess;
	ret = vaccel_session_init(&sess, 0);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(sess.id == 1);

	/* Register resources */
	ret = vaccel_resource_register(&res, &sess);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(res.refcount == 1);

	ret = vaccel_resource_register(alloc_res, &sess);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(alloc_res->refcount == 1);

	/* Get by id */
	struct vaccel_resource *found_by_id;
	ret = vaccel_resource_get_by_id(&found_by_id, 1);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(found_by_id == &res);

	ret = vaccel_resource_get_by_id(&found_by_id, 2);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(found_by_id == alloc_res);

	/* Has resource */
	REQUIRE(vaccel_session_has_resource(&sess, &res));
	REQUIRE(vaccel_session_has_resource(&sess, alloc_res));

	/* Unregister resources */
	ret = vaccel_resource_unregister(&res, &sess);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE_FALSE(vaccel_session_has_resource(&sess, &res));
	REQUIRE(res.refcount == 0);

	ret = vaccel_resource_unregister(alloc_res, &sess);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE_FALSE(vaccel_session_has_resource(&sess, alloc_res));
	REQUIRE(alloc_res->refcount == 0);

	/* Free resource */
	ret = vaccel_resource_delete(alloc_res);
	REQUIRE(ret == VACCEL_OK);

	/* Release resource */
	ret = vaccel_resource_release(&res);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(res.rundir == nullptr);
	REQUIRE(res.files == nullptr);
	REQUIRE(res.nr_files == 0);
	REQUIRE(res.paths == nullptr);
	REQUIRE(res.nr_paths == 0);
	REQUIRE(res.id <= 0);

	/* Release session */
	ret = vaccel_session_release(&sess);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(sess.id <= 0);

	/* Release files */
	ret = vaccel_file_release(&f1);
	REQUIRE(ret == VACCEL_OK);

	ret = vaccel_file_release(&f2);
	REQUIRE(ret == VACCEL_OK);

	free(path1);
	free(path2);
}

// Test case for resource init failures
TEST_CASE("resource_init_fail", "[core][resource]")
{
	int ret;
	struct vaccel_resource res;
	struct vaccel_resource *alloc_res;
	vaccel_resource_t test_type = VACCEL_RESOURCE_LIB;

	char *test_path = abs_path(BUILD_ROOT, "examples/libmytestlib.so");
	const char *paths[2] = { test_path, nullptr };

	size_t len;
	unsigned char *buff;
	ret = fs_file_read(test_path, (void **)&buff, &len);
	REQUIRE(ret == VACCEL_OK);

	struct vaccel_file f;
	ret = vaccel_file_init(&f, test_path);
	REQUIRE(ret == VACCEL_OK);
	ret = vaccel_file_read(&f);
	REQUIRE(ret == VACCEL_OK);
	const struct vaccel_file *files[2] = { &f, nullptr };

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
						    test_type, nullptr);
		REQUIRE(ret == VACCEL_EINVAL);
		ret = vaccel_resource_init_from_buf(&res, nullptr, len,
						    test_type, nullptr);
		REQUIRE(ret == VACCEL_EINVAL);
		ret = vaccel_resource_init_from_buf(&res, &buff, 0, test_type,
						    nullptr);
		REQUIRE(ret == VACCEL_EINVAL);
		ret = vaccel_resource_init_from_buf(
			&res, &buff, len, VACCEL_RESOURCE_MAX, nullptr);
		REQUIRE(ret == VACCEL_EINVAL);

		ret = vaccel_resource_init_from_files(nullptr, files, 1,
						      test_type);
		REQUIRE(ret == VACCEL_EINVAL);
		ret = vaccel_resource_init_from_files(&res, nullptr, 1,
						      test_type);
		REQUIRE(ret == VACCEL_EINVAL);
		ret = vaccel_resource_init_from_files(&res, files, 0,
						      test_type);
		REQUIRE(ret == VACCEL_EINVAL);
		ret = vaccel_resource_init_from_files(&res, files, 1,
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
					       nullptr);
		REQUIRE(ret == VACCEL_EINVAL);
		ret = vaccel_resource_from_buf(&alloc_res, nullptr, len,
					       test_type, nullptr);
		REQUIRE(ret == VACCEL_EINVAL);
		ret = vaccel_resource_from_buf(&alloc_res, &buff, 0, test_type,
					       nullptr);
		REQUIRE(ret == VACCEL_EINVAL);
		ret = vaccel_resource_from_buf(&alloc_res, &buff, len,
					       VACCEL_RESOURCE_MAX, nullptr);
		REQUIRE(ret == VACCEL_EINVAL);

		ret = vaccel_resource_from_files(nullptr, files, 1, test_type);
		REQUIRE(ret == VACCEL_EINVAL);
		ret = vaccel_resource_from_files(&alloc_res, nullptr, 1,
						 test_type);
		REQUIRE(ret == VACCEL_EINVAL);
		ret = vaccel_resource_from_files(&alloc_res, files, 0,
						 test_type);
		REQUIRE(ret == VACCEL_EINVAL);
		ret = vaccel_resource_from_files(&alloc_res, files, 1,
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

	SECTION("invalid files")
	{
		ret = vaccel_resource_init_from_files(&res, files, 2,
						      test_type);
		REQUIRE(ret == VACCEL_EINVAL);
		REQUIRE(res.files == nullptr);
		REQUIRE(res.nr_files == 0);

		ret = vaccel_resource_from_files(&alloc_res, files, 2,
						 test_type);
		REQUIRE(ret == VACCEL_EINVAL);
		REQUIRE(res.files == nullptr);
		REQUIRE(res.nr_files == 0);
	}

	ret = vaccel_file_release(&f);
	REQUIRE(ret == VACCEL_OK);

	free(buff);
	free(test_path);
}

// Test case for resource release failures
TEST_CASE("resource_release_fail", "[core][resource]")
{
	int ret;
	struct vaccel_resource res;
	vaccel_resource_t test_type = VACCEL_RESOURCE_LIB;
	char *test_path = abs_path(BUILD_ROOT, "examples/libmytestlib.so");

	ret = vaccel_resource_init(&res, test_path, test_type);
	REQUIRE(ret == VACCEL_OK);

	struct vaccel_session sess;
	ret = vaccel_session_init(&sess, 0);
	REQUIRE(ret == VACCEL_OK);

	ret = vaccel_resource_register(&res, &sess);
	REQUIRE(ret == VACCEL_OK);

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

	ret = vaccel_session_release(&sess);
	REQUIRE(ret == VACCEL_OK);

	free(test_path);
}

// Test case for resource register/unregister to/from multiple sessions
TEST_CASE("resource_multiple_sessions", "[core][resource]")
{
	int ret;
	struct vaccel_resource res;
	char *test_path = abs_path(BUILD_ROOT, "examples/libmytestlib.so");
	vaccel_resource_t test_type = VACCEL_RESOURCE_LIB;

	const size_t nr_sessions = 4;
	struct vaccel_session sessions[nr_sessions];

	ret = vaccel_resource_init(&res, test_path, test_type);
	REQUIRE(ret == VACCEL_OK);

	for (auto &sess : sessions) {
		ret = vaccel_session_init(&sess, 0);
		REQUIRE(ret == VACCEL_OK);

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
	for (auto &sess : sessions)
		REQUIRE_FALSE(vaccel_session_has_resource(&sess, &res));

	for (size_t i = nr_sessions - 1; i < (size_t)-1; i--) {
		ret = vaccel_session_release(&sessions[i]);
		REQUIRE(ret == VACCEL_OK);
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
	vaccel_resource_t test_type = VACCEL_RESOURCE_LIB;

	struct vaccel_session sess;
	ret = vaccel_session_init(&sess, 0);
	REQUIRE(ret == VACCEL_OK);

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

	ret = vaccel_session_release(&sess);
	REQUIRE(ret == VACCEL_OK);

	free(test_path);
}

// Test case for resource unregister failures
TEST_CASE("resource_unregister_fail", "[core][resource]")
{
	int ret;
	struct vaccel_resource res;
	char *test_path = abs_path(BUILD_ROOT, "examples/libmytestlib.so");
	vaccel_resource_t test_type = VACCEL_RESOURCE_LIB;

	struct vaccel_session sess;
	ret = vaccel_session_init(&sess, 0);
	REQUIRE(ret == VACCEL_OK);

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

	ret = vaccel_session_release(&sess);
	REQUIRE(ret == VACCEL_OK);

	free(test_path);
}

// Test case for finding a resource by ID failure
TEST_CASE("resource_find_by_id_fail", "[core][resource]")
{
	struct vaccel_resource *test_res = nullptr;
	vaccel_id_t test_id = 0;

	// Attempt to find a resource by ID which fails (ID of 0 doesn't exist -
	// starts at 1)
	int ret = vaccel_resource_get_by_id(&test_res, test_id);
	REQUIRE(ret == VACCEL_EINVAL);
}

// Test case for resource component not bootstrapped
TEST_CASE("resources_not_bootstrapped", "[core][resource]")
{
	int ret;
	struct vaccel_resource res;
	vaccel_resource_t test_type = VACCEL_RESOURCE_LIB;

	// cleanup here so resources are not bootstrapped
	ret = resources_cleanup();
	REQUIRE(ret == VACCEL_OK);

	ret = vaccel_resource_init(&res, nullptr, test_type);
	REQUIRE(ret == VACCEL_EPERM);

	ret = vaccel_resource_init_multi(&res, nullptr, 0, test_type);
	REQUIRE(ret == VACCEL_EPERM);

	ret = vaccel_resource_init_from_buf(&res, nullptr, 0, test_type,
					    nullptr);
	REQUIRE(ret == VACCEL_EPERM);

	ret = vaccel_resource_init_from_files(&res, nullptr, 0, test_type);
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
