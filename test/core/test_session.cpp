// SPDX-License-Identifier: Apache-2.0

/*
 * The code below performs unit testing to sessions.
 *
 * 1) vaccel_session_init()
 * 2) vaccel_session_release()
 * 3) vaccel_session_new()
 * 4) vaccel_session_delete()
 * 5) vaccel_session_update()
 * 6) session_register_resource()
 * 7) session_unregister_resource()
 * 8) vaccel_session_has_resource()
 * 9) vaccel_session_resource_by_type()
 * 10) vaccel_session_resource_by_id()
 * 11) vaccel_session_resources_by_type()
 *
 */

#include "utils.hpp"
#include "vaccel.h"
#include <catch.hpp>
#include <cerrno>
#include <cinttypes>
#include <cstdio>
#include <cstdlib>
#include <fff.h>
#include <mock_virtio.hpp>
#include <pthread.h>
#include <unistd.h>

DEFINE_FFF_GLOBALS;

TEST_CASE("vaccel_session_init", "[core][session]")
{
	int ret;

	struct vaccel_session sess;
	ret = vaccel_session_init(&sess, 1);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(sess.id > 0);
	REQUIRE(sess.hint == 1);
	for (auto &resource : sess.resources)
		REQUIRE(list_empty(&resource));
	REQUIRE(sess.priv == nullptr);

	SECTION("invalid arguments")
	{
		ret = vaccel_session_init(nullptr, 1);
		REQUIRE(ret == VACCEL_EINVAL);
	}

	REQUIRE(vaccel_session_release(&sess) == VACCEL_OK);
}

TEST_CASE("vaccel_session_release", "[core][session]")
{
	int ret;

	struct vaccel_session sess;
	REQUIRE(vaccel_session_init(&sess, 1) == VACCEL_OK);

	SECTION("invalid arguments")
	{
		ret = vaccel_session_release(nullptr);
		REQUIRE(ret == VACCEL_EINVAL);
		REQUIRE(sess.id > 0);
		REQUIRE(sess.hint == 1);
		for (auto &resource : sess.resources)
			REQUIRE(list_empty(&resource));
		REQUIRE(sess.priv == nullptr);
	}

	ret = vaccel_session_release(&sess);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(sess.id == 0);
	REQUIRE(sess.hint == 1);
	for (auto &resource : sess.resources)
		REQUIRE(list_empty(&resource));
	REQUIRE(sess.priv == nullptr);
}

TEST_CASE("vaccel_session_new", "[core][session]")
{
	int ret;

	struct vaccel_session *sess;
	ret = vaccel_session_new(&sess, 1);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(sess->id);
	REQUIRE(sess->hint == 1);
	for (auto &resource : sess->resources)
		REQUIRE(list_empty(&resource));
	REQUIRE(sess->priv == nullptr);

	SECTION("invalid arguments")
	{
		ret = vaccel_session_new(nullptr, 1);
		REQUIRE(ret == VACCEL_EINVAL);
	}

	REQUIRE(vaccel_session_delete(sess) == VACCEL_OK);
}

TEST_CASE("vaccel_session_delete", "[core][session]")
{
	int ret;

	struct vaccel_session *sess;
	REQUIRE(vaccel_session_new(&sess, 1) == VACCEL_OK);

	SECTION("invalid arguments")
	{
		ret = vaccel_session_delete(nullptr);
		REQUIRE(ret == VACCEL_EINVAL);
		REQUIRE(sess->id);
		REQUIRE(sess->hint == 1);
		for (auto &resource : sess->resources)
			REQUIRE(list_empty(&resource));
		REQUIRE(sess->priv == nullptr);
	}

	ret = vaccel_session_delete(sess);
	REQUIRE(ret == VACCEL_OK);
}

TEST_CASE("vaccel_session_update", "[core][session]")
{
	int ret;

	struct vaccel_session sess;
	REQUIRE(vaccel_session_init(&sess, 1) == VACCEL_OK);

	ret = vaccel_session_update(&sess, 2);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(sess.hint == 2);

	SECTION("invalid arguments")
	{
		ret = vaccel_session_update(nullptr, 1);
		REQUIRE(ret == VACCEL_EINVAL);
	}

	REQUIRE(vaccel_session_release(&sess) == VACCEL_OK);
}

TEST_CASE("vaccel_session_has_resource", "[core][session]")
{
	struct vaccel_session sess;
	REQUIRE(vaccel_session_init(&sess, 1) == VACCEL_OK);

	char *lib_path = abs_path(BUILD_ROOT, "examples/libmytestlib.so");
	struct vaccel_resource res;
	REQUIRE(vaccel_resource_init(&res, lib_path, VACCEL_RESOURCE_LIB) ==
		VACCEL_OK);
	REQUIRE(vaccel_resource_register(&res, &sess) == VACCEL_OK);
	REQUIRE(!list_empty(&sess.resources[VACCEL_RESOURCE_LIB]));

	REQUIRE(vaccel_session_has_resource(&sess, &res));

	SECTION("invalid arguments")
	{
		REQUIRE_FALSE(vaccel_session_has_resource(nullptr, &res));
		REQUIRE_FALSE(vaccel_session_has_resource(&sess, nullptr));
	}

	REQUIRE(vaccel_resource_unregister(&res, &sess) == VACCEL_OK);
	REQUIRE(vaccel_resource_release(&res) == VACCEL_OK);

	REQUIRE_FALSE(vaccel_session_has_resource(&sess, &res));

	REQUIRE(vaccel_session_release(&sess) == VACCEL_OK);
	free(lib_path);
}

TEST_CASE("session_ops", "[core][session]")
{
	struct vaccel_session sess;
	REQUIRE(vaccel_session_init(&sess, 1) == VACCEL_OK);

	struct vaccel_session *alloc_sess;
	REQUIRE(vaccel_session_new(&alloc_sess, 1) == VACCEL_OK);

	char *lib_path = abs_path(BUILD_ROOT, "examples/libmytestlib.so");
	struct vaccel_resource res;
	REQUIRE(vaccel_resource_init(&res, lib_path, VACCEL_RESOURCE_LIB) ==
		VACCEL_OK);

	REQUIRE(vaccel_session_update(&sess, 2) == VACCEL_OK);
	REQUIRE(sess.hint == 2);
	REQUIRE(vaccel_session_update(alloc_sess, 2) == VACCEL_OK);
	REQUIRE(alloc_sess->hint == 2);

	REQUIRE(vaccel_resource_register(&res, &sess) == VACCEL_OK);
	REQUIRE(vaccel_resource_register(&res, alloc_sess) == VACCEL_OK);

	REQUIRE(vaccel_session_has_resource(&sess, &res));
	REQUIRE(vaccel_session_has_resource(alloc_sess, &res));

	REQUIRE(vaccel_session_update(&sess, 3) == VACCEL_OK);
	REQUIRE(sess.hint == 3);
	REQUIRE(vaccel_session_update(alloc_sess, 3) == VACCEL_OK);
	REQUIRE(alloc_sess->hint == 3);

	REQUIRE(vaccel_resource_unregister(&res, &sess) == VACCEL_OK);
	REQUIRE(vaccel_resource_unregister(&res, alloc_sess) == VACCEL_OK);
	REQUIRE(vaccel_resource_release(&res) == VACCEL_OK);

	REQUIRE_FALSE(vaccel_session_has_resource(&sess, &res));
	REQUIRE_FALSE(vaccel_session_has_resource(alloc_sess, &res));

	REQUIRE(vaccel_session_release(&sess) == VACCEL_OK);
	REQUIRE(vaccel_session_delete(alloc_sess) == VACCEL_OK);
	free(lib_path);
}

TEST_CASE("session_virtio", "[core][session]")
{
	struct vaccel_session sess;

	SECTION("not_registered")
	{
		REQUIRE(vaccel_session_init(&sess, 1 | VACCEL_PLUGIN_REMOTE) ==
			VACCEL_ENOTSUP);
	}

	auto *virtio_plugin = mock_virtio_plugin_virtio();
	REQUIRE(plugin_register(virtio_plugin) == VACCEL_OK);

	REQUIRE(vaccel_session_init(&sess,
				    VACCEL_PLUGIN_CPU | VACCEL_PLUGIN_REMOTE) ==
		VACCEL_OK);

	struct vaccel_session *alloc_sess;
	REQUIRE(vaccel_session_new(&alloc_sess,
				   VACCEL_PLUGIN_CPU | VACCEL_PLUGIN_REMOTE) ==
		VACCEL_OK);

	char *lib_path = abs_path(BUILD_ROOT, "examples/libmytestlib.so");
	struct vaccel_resource res;
	REQUIRE(vaccel_resource_init(&res, lib_path, VACCEL_RESOURCE_LIB) ==
		VACCEL_OK);

	REQUIRE(vaccel_session_update(&sess, VACCEL_PLUGIN_GPU) == VACCEL_OK);
	REQUIRE(sess.hint == VACCEL_PLUGIN_GPU);
	REQUIRE(vaccel_session_update(alloc_sess, VACCEL_PLUGIN_GPU) ==
		VACCEL_OK);
	REQUIRE(alloc_sess->hint == VACCEL_PLUGIN_GPU);

	REQUIRE(vaccel_resource_register(&res, &sess) == VACCEL_OK);
	REQUIRE(vaccel_resource_register(&res, alloc_sess) == VACCEL_OK);

	REQUIRE(vaccel_session_has_resource(&sess, &res));
	REQUIRE(vaccel_session_has_resource(alloc_sess, &res));

	REQUIRE(vaccel_session_update(&sess, VACCEL_PLUGIN_FPGA) == VACCEL_OK);
	REQUIRE(sess.hint == VACCEL_PLUGIN_FPGA);
	REQUIRE(vaccel_session_update(alloc_sess, VACCEL_PLUGIN_FPGA) ==
		VACCEL_OK);
	REQUIRE(alloc_sess->hint == VACCEL_PLUGIN_FPGA);

	REQUIRE(vaccel_resource_unregister(&res, &sess) == VACCEL_OK);
	REQUIRE(vaccel_resource_unregister(&res, alloc_sess) == VACCEL_OK);
	REQUIRE(vaccel_resource_release(&res) == VACCEL_OK);

	REQUIRE_FALSE(vaccel_session_has_resource(&sess, &res));
	REQUIRE_FALSE(vaccel_session_has_resource(alloc_sess, &res));

	REQUIRE(vaccel_session_release(&sess) == VACCEL_OK);
	REQUIRE(vaccel_session_delete(alloc_sess) == VACCEL_OK);

	REQUIRE(plugin_unregister(virtio_plugin) == VACCEL_OK);
	free(lib_path);
}

TEST_CASE("vaccel_session_resource_by_type", "[core][session]")
{
	int ret;
	struct vaccel_resource *res_ptr;
	struct vaccel_session sess;

	SECTION("invalid arguments")
	{
		ret = vaccel_session_resource_by_type(nullptr, &res_ptr,
						      VACCEL_RESOURCE_LIB);
		REQUIRE(ret == VACCEL_EINVAL);

		ret = vaccel_session_resource_by_type(&sess, nullptr,
						      VACCEL_RESOURCE_LIB);
		REQUIRE(ret == VACCEL_EINVAL);

		ret = vaccel_session_resource_by_type(&sess, &res_ptr,
						      VACCEL_RESOURCE_MAX);
		REQUIRE(ret == VACCEL_EINVAL);
	}

	REQUIRE(vaccel_session_init(&sess, 0) == VACCEL_OK);

	SECTION("no resource")
	{
		ret = vaccel_session_resource_by_type(&sess, &res_ptr,
						      VACCEL_RESOURCE_LIB);
		REQUIRE(ret == VACCEL_ENOENT);

		ret = vaccel_session_resource_by_type(&sess, &res_ptr,
						      VACCEL_RESOURCE_MODEL);
		REQUIRE(ret == VACCEL_ENOENT);

		ret = vaccel_session_resource_by_type(&sess, &res_ptr,
						      VACCEL_RESOURCE_DATA);
		REQUIRE(ret == VACCEL_ENOENT);
	}

	char *lib_path = abs_path(BUILD_ROOT, "examples/libmytestlib.so");
	struct vaccel_resource res;
	REQUIRE(vaccel_resource_init(&res, lib_path, VACCEL_RESOURCE_LIB) ==
		VACCEL_OK);

	SECTION("resource not registered")
	{
		ret = vaccel_session_resource_by_type(&sess, &res_ptr,
						      VACCEL_RESOURCE_LIB);
		REQUIRE(ret == VACCEL_ENOENT);
	}

	REQUIRE(vaccel_resource_register(&res, &sess) == VACCEL_OK);

	ret = vaccel_session_resource_by_type(&sess, &res_ptr,
					      VACCEL_RESOURCE_LIB);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(res_ptr == &res);

	SECTION("invalid resource type")
	{
		ret = vaccel_session_resource_by_type(&sess, &res_ptr,
						      VACCEL_RESOURCE_MODEL);
		REQUIRE(ret == VACCEL_ENOENT);

		ret = vaccel_session_resource_by_type(&sess, &res_ptr,
						      VACCEL_RESOURCE_DATA);
		REQUIRE(ret == VACCEL_ENOENT);
	}

	REQUIRE(vaccel_resource_unregister(&res, &sess) == VACCEL_OK);
	REQUIRE(vaccel_resource_release(&res) == VACCEL_OK);

	REQUIRE(vaccel_session_release(&sess) == VACCEL_OK);
	free(lib_path);
}

TEST_CASE("vaccel_session_resource_by_id", "[core][session]")
{
	int ret;
	struct vaccel_resource *res_ptr;
	struct vaccel_session sess;

	SECTION("invalid arguments")
	{
		ret = vaccel_session_resource_by_id(nullptr, &res_ptr, 1);
		REQUIRE(ret == VACCEL_EINVAL);

		ret = vaccel_session_resource_by_id(&sess, nullptr, 1);
		REQUIRE(ret == VACCEL_EINVAL);

		ret = vaccel_session_resource_by_id(&sess, &res_ptr, 0);
		REQUIRE(ret == VACCEL_EINVAL);
	}

	REQUIRE(vaccel_session_init(&sess, 0) == VACCEL_OK);

	SECTION("no resource")
	{
		ret = vaccel_session_resource_by_id(&sess, &res_ptr, 1);
		REQUIRE(ret == VACCEL_ENOENT);
	}

	char *lib_path = abs_path(BUILD_ROOT, "examples/libmytestlib.so");
	struct vaccel_resource res;
	REQUIRE(vaccel_resource_init(&res, lib_path, VACCEL_RESOURCE_LIB) ==
		VACCEL_OK);

	SECTION("resource not registered")
	{
		ret = vaccel_session_resource_by_id(&sess, &res_ptr, res.id);
		REQUIRE(ret == VACCEL_ENOENT);
	}

	REQUIRE(vaccel_resource_register(&res, &sess) == VACCEL_OK);

	ret = vaccel_session_resource_by_id(&sess, &res_ptr, res.id);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(res_ptr == &res);

	SECTION("invalid resource id")
	{
		ret = vaccel_session_resource_by_id(&sess, &res_ptr,
						    res.id + 1);
		REQUIRE(ret == VACCEL_ENOENT);
	}

	REQUIRE(vaccel_resource_unregister(&res, &sess) == VACCEL_OK);
	REQUIRE(vaccel_resource_release(&res) == VACCEL_OK);

	REQUIRE(vaccel_session_release(&sess) == VACCEL_OK);
	free(lib_path);
}

TEST_CASE("vaccel_session_resources_by_type", "[core][session]")
{
	int ret;
	struct vaccel_resource **res_ptr;
	struct vaccel_session sess;
	size_t nr_found;

	SECTION("invalid arguments")
	{
		ret = vaccel_session_resources_by_type(
			nullptr, &res_ptr, &nr_found, VACCEL_RESOURCE_LIB);
		REQUIRE(ret == VACCEL_EINVAL);

		ret = vaccel_session_resources_by_type(
			&sess, nullptr, &nr_found, VACCEL_RESOURCE_LIB);
		REQUIRE(ret == VACCEL_EINVAL);

		ret = vaccel_session_resources_by_type(&sess, &res_ptr, nullptr,
						       VACCEL_RESOURCE_LIB);
		REQUIRE(ret == VACCEL_EINVAL);

		ret = vaccel_session_resources_by_type(
			&sess, &res_ptr, &nr_found, VACCEL_RESOURCE_MAX);
		REQUIRE(ret == VACCEL_EINVAL);
	}

	REQUIRE(vaccel_session_init(&sess, 0) == VACCEL_OK);

	SECTION("no resource")
	{
		ret = vaccel_session_resources_by_type(
			&sess, &res_ptr, &nr_found, VACCEL_RESOURCE_LIB);
		REQUIRE(ret == VACCEL_ENOENT);
		REQUIRE(nr_found == 0);

		ret = vaccel_session_resources_by_type(
			&sess, &res_ptr, &nr_found, VACCEL_RESOURCE_MODEL);
		REQUIRE(ret == VACCEL_ENOENT);
		REQUIRE(nr_found == 0);

		ret = vaccel_session_resources_by_type(
			&sess, &res_ptr, &nr_found, VACCEL_RESOURCE_DATA);
		REQUIRE(ret == VACCEL_ENOENT);
		REQUIRE(nr_found == 0);
	}

	char *lib_path = abs_path(BUILD_ROOT, "examples/libmytestlib.so");
	struct vaccel_resource res;
	REQUIRE(vaccel_resource_init(&res, lib_path, VACCEL_RESOURCE_LIB) ==
		VACCEL_OK);

	SECTION("resource not registered")
	{
		ret = vaccel_session_resources_by_type(
			&sess, &res_ptr, &nr_found, VACCEL_RESOURCE_LIB);
		REQUIRE(ret == VACCEL_ENOENT);
		REQUIRE(nr_found == 0);
	}

	REQUIRE(vaccel_resource_register(&res, &sess) == VACCEL_OK);

	ret = vaccel_session_resources_by_type(&sess, &res_ptr, &nr_found,
					       VACCEL_RESOURCE_LIB);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(nr_found == 1);
	REQUIRE(res_ptr[0] == &res);

	free(res_ptr);

	/* Success with more than 1 resource */
	struct vaccel_resource res2;
	REQUIRE(vaccel_resource_init(&res2, lib_path, VACCEL_RESOURCE_LIB) ==
		VACCEL_OK);
	REQUIRE(vaccel_resource_register(&res2, &sess) == VACCEL_OK);

	ret = vaccel_session_resources_by_type(&sess, &res_ptr, &nr_found,
					       VACCEL_RESOURCE_LIB);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(nr_found == 2);
	REQUIRE(res_ptr[0] == &res);
	REQUIRE(res_ptr[1] == &res2);

	free(res_ptr);

	REQUIRE(vaccel_resource_unregister(&res2, &sess) == VACCEL_OK);
	REQUIRE(vaccel_resource_release(&res2) == VACCEL_OK);

	SECTION("invalid resource type")
	{
		ret = vaccel_session_resources_by_type(
			&sess, &res_ptr, &nr_found, VACCEL_RESOURCE_MODEL);
		REQUIRE(ret == VACCEL_ENOENT);

		ret = vaccel_session_resources_by_type(
			&sess, &res_ptr, &nr_found, VACCEL_RESOURCE_DATA);
		REQUIRE(ret == VACCEL_ENOENT);
	}

	REQUIRE(vaccel_resource_unregister(&res, &sess) == VACCEL_OK);
	REQUIRE(vaccel_resource_release(&res) == VACCEL_OK);

	REQUIRE(vaccel_session_release(&sess) == VACCEL_OK);
	free(lib_path);
}

enum {
	TEST_THREADS_NUM = 50,
	TEST_THREAD_SESSIONS_NUM = 30,
	TEST_SESSIONS_NUM = 20,
	TEST_RESOURCES_NUM = 10
};

struct thread_data {
	size_t id;
	struct vaccel_session *sessions;
	struct vaccel_resource *resources;
};

static auto init_and_release_session(void *arg) -> void *
{
	auto *data = (struct thread_data *)arg;
	struct vaccel_session *sess = data->sessions;

	for (int i = 0; i < TEST_THREAD_SESSIONS_NUM; i++) {
		REQUIRE(vaccel_session_init(sess, 0) == VACCEL_OK);
		printf("Thread %zu: Created session %" PRId64 "\n", data->id,
		       sess->id);

		// Add random delay to simulate work
		usleep(rand() % 1000);

		vaccel_id_t const sess_id = sess->id;
		REQUIRE(vaccel_session_release(sess) == VACCEL_OK);
		printf("Thread %zu: Deleted session %" PRId64 "\n", data->id,
		       sess_id);
	}
	return nullptr;
}

TEST_CASE("session_init_and_release_concurrent", "[core][session]")
{
	pthread_t threads[TEST_THREADS_NUM];
	struct thread_data thread_data[TEST_THREADS_NUM];
	struct vaccel_session sess[TEST_THREADS_NUM];

	for (size_t i = 0; i < TEST_THREADS_NUM; i++) {
		thread_data[i].id = i;
		thread_data[i].sessions = &sess[i];
		thread_data[i].resources = nullptr;
		pthread_create(&threads[i], nullptr, init_and_release_session,
			       &thread_data[i]);
	}

	for (unsigned long const thread : threads)
		pthread_join(thread, nullptr);
}

static auto register_and_find_resources(void *arg) -> void *
{
	auto *data = (struct thread_data *)arg;
	struct vaccel_session *sess = data->sessions;
	struct vaccel_resource *res = data->resources;

	size_t sess_iter = 0;
	size_t sess_iter_next = 1;
	for (size_t i = 0; i < TEST_RESOURCES_NUM; i++) {
		if (sess_iter >= TEST_SESSIONS_NUM) {
			sess_iter = 0;
			sess_iter_next = 1;
		}

		REQUIRE(vaccel_resource_register(&res[i], &sess[sess_iter]) ==
			VACCEL_OK);
		printf("Thread %zu: Registered resource %" PRId64
		       " with session %" PRId64 "\n",
		       data->id, res[i].id, sess[sess_iter].id);

		// Add random delay to simulate work
		usleep(rand() % 1000);

		struct vaccel_resource *found_res;
		auto const res_id = (rand() % (res[TEST_RESOURCES_NUM - 1].id -
					       res[0].id + 1) +
				     res[0].id);
		int ret = vaccel_session_resource_by_id(&sess[sess_iter_next],
							&found_res, res_id);
		REQUIRE((ret == VACCEL_OK || ret == VACCEL_ENOENT));

		ret = vaccel_session_resource_by_type(
			&sess[sess_iter_next], &found_res, VACCEL_RESOURCE_LIB);
		REQUIRE((ret == VACCEL_OK || ret == VACCEL_ENOENT));

		struct vaccel_resource **found = nullptr;
		size_t nr_found = 0;
		ret = vaccel_session_resources_by_type(&sess[sess_iter_next],
						       &found, &nr_found,
						       VACCEL_RESOURCE_LIB);
		REQUIRE((ret == VACCEL_OK || ret == VACCEL_ENOENT));
		if ((found != nullptr) && nr_found > 0)
			free(found);

		REQUIRE(vaccel_resource_unregister(
				&res[i], &sess[sess_iter++]) == VACCEL_OK);
		printf("Thread %zu: Unregistered resource %" PRId64
		       " from session %" PRId64 "\n",
		       data->id, res[i].id, sess[sess_iter].id);
	}
	return nullptr;
}

TEST_CASE("session_resource_find_concurrent", "[core][session]")
{
	char *lib_path = abs_path(BUILD_ROOT, "examples/libmytestlib.so");
	const auto nr_res =
		(size_t)TEST_THREADS_NUM * (size_t)TEST_RESOURCES_NUM;
	struct vaccel_resource resources[nr_res];
	struct vaccel_session sessions[TEST_SESSIONS_NUM];

	for (auto &res : resources)
		REQUIRE(vaccel_resource_init(&res, lib_path,
					     VACCEL_RESOURCE_LIB) == VACCEL_OK);

	for (auto &sess : sessions)
		REQUIRE(vaccel_session_init(&sess, 0) == VACCEL_OK);

	pthread_t threads[TEST_THREADS_NUM];
	struct thread_data thread_data[TEST_THREADS_NUM];

	for (size_t i = 0; i < TEST_THREADS_NUM; i++) {
		thread_data[i].id = i;
		thread_data[i].sessions = sessions;
		thread_data[i].resources = &resources[i * TEST_RESOURCES_NUM];
		pthread_create(&threads[i], nullptr,
			       register_and_find_resources, &thread_data[i]);
	}

	for (unsigned long const thread : threads)
		pthread_join(thread, nullptr);

	for (auto &sess : sessions)
		REQUIRE(vaccel_session_release(&sess) == VACCEL_OK);

	for (auto &res : resources)
		REQUIRE(vaccel_resource_release(&res) == VACCEL_OK);

	free(lib_path);
}
