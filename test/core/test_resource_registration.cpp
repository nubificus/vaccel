// SPDX-License-Identifier: Apache-2.0

/*
 * The code below performs unit testing to `resource_registration`.
 *
 * 1) resource_registration_new()
 * 2) resource_registration_delete()
 * 3) resource_registration_link()
 * 4) resource_registration_unlink()
 * 5) resource_registration_find()
 * 6) resource_registration_find_and_unlink()
 * 7) resource_registration_foreach_session()
 * 8) resource_registration_foreach_resource()
 *
 */

#include "utils.hpp"
#include "vaccel.h"
#include <atomic>
#include <catch2/catch_test_macros.hpp>
#include <cerrno>
#include <cinttypes>
#include <cstdio>
#include <cstdlib>
#include <pthread.h>
#include <unistd.h>

TEST_CASE("resource_registration_new", "[core][resource_registration]")
{
	int ret;
	char *lib_path = abs_path(BUILD_ROOT, "examples/libmytestlib.so");
	struct vaccel_session sess;
	struct vaccel_resource res;

	REQUIRE(vaccel_session_init(&sess, 0) == VACCEL_OK);
	REQUIRE(vaccel_resource_init(&res, lib_path, VACCEL_RESOURCE_LIB) ==
		VACCEL_OK);

	struct resource_registration *reg;
	ret = resource_registration_new(&reg, &res, &sess);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(reg->resource == &res);
	REQUIRE(reg->session == &sess);

	SECTION("invalid arguments")
	{
		ret = resource_registration_new(nullptr, &res, &sess);
		REQUIRE(ret == VACCEL_EINVAL);

		ret = resource_registration_new(&reg, nullptr, &sess);
		REQUIRE(ret == VACCEL_EINVAL);

		ret = resource_registration_new(&reg, &res, nullptr);
		REQUIRE(ret == VACCEL_EINVAL);
	}

	REQUIRE(resource_registration_delete(reg) == VACCEL_OK);
	REQUIRE(vaccel_resource_release(&res) == VACCEL_OK);
	REQUIRE(vaccel_session_release(&sess) == VACCEL_OK);
	free(lib_path);
}

TEST_CASE("resource_registration_delete", "[core][resource_registration]")
{
	int ret;
	char *lib_path = abs_path(BUILD_ROOT, "examples/libmytestlib.so");
	struct vaccel_session sess;
	struct vaccel_resource res;

	REQUIRE(vaccel_session_init(&sess, 0) == VACCEL_OK);
	REQUIRE(vaccel_resource_init(&res, lib_path, VACCEL_RESOURCE_LIB) ==
		VACCEL_OK);

	struct resource_registration *reg;
	REQUIRE(resource_registration_new(&reg, &res, &sess) == VACCEL_OK);

	SECTION("invalid arguments")
	{
		ret = resource_registration_delete(nullptr);
		REQUIRE(ret == VACCEL_EINVAL);
	}

	ret = resource_registration_delete(reg);
	REQUIRE(ret == VACCEL_OK);

	REQUIRE(vaccel_resource_release(&res) == VACCEL_OK);
	REQUIRE(vaccel_session_release(&sess) == VACCEL_OK);
	free(lib_path);
}

TEST_CASE("resource_registration_link", "[core][resource_registration]")
{
	int ret;
	char *lib_path = abs_path(BUILD_ROOT, "examples/libmytestlib.so");
	struct vaccel_session sess;
	struct vaccel_resource res;

	REQUIRE(vaccel_session_init(&sess, 0) == VACCEL_OK);
	REQUIRE(vaccel_resource_init(&res, lib_path, VACCEL_RESOURCE_LIB) ==
		VACCEL_OK);

	struct resource_registration *reg;
	REQUIRE(resource_registration_new(&reg, &res, &sess) == VACCEL_OK);

	ret = resource_registration_link(reg);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(!list_empty(&res.sessions));
	REQUIRE(!list_empty(&sess.resources[res.type]));
	REQUIRE(sess.resource_counts[res.type] == 1);
	REQUIRE(atomic_load(&res.refcount) == 1);

	SECTION("invalid arguments")
	{
		ret = resource_registration_link(nullptr);
		REQUIRE(ret == VACCEL_EINVAL);
	}

	REQUIRE(resource_registration_unlink(reg) == VACCEL_OK);
	REQUIRE(resource_registration_delete(reg) == VACCEL_OK);

	REQUIRE(vaccel_resource_release(&res) == VACCEL_OK);
	REQUIRE(vaccel_session_release(&sess) == VACCEL_OK);
	free(lib_path);
}

TEST_CASE("resource_registration_unlink", "[core][resource_registration]")
{
	int ret;
	char *lib_path = abs_path(BUILD_ROOT, "examples/libmytestlib.so");
	struct vaccel_session sess;
	struct vaccel_resource res;

	REQUIRE(vaccel_session_init(&sess, 0) == VACCEL_OK);
	REQUIRE(vaccel_resource_init(&res, lib_path, VACCEL_RESOURCE_LIB) ==
		VACCEL_OK);

	struct resource_registration *reg;
	REQUIRE(resource_registration_new(&reg, &res, &sess) == VACCEL_OK);
	REQUIRE(resource_registration_link(reg) == VACCEL_OK);

	SECTION("invalid arguments")
	{
		ret = resource_registration_unlink(nullptr);
		REQUIRE(ret == VACCEL_EINVAL);
	}

	ret = resource_registration_unlink(reg);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(list_empty(&res.sessions));
	REQUIRE(list_empty(&sess.resources[res.type]));
	REQUIRE(sess.resource_counts[res.type] == 0);
	REQUIRE(atomic_load(&res.refcount) == 0);

	REQUIRE(resource_registration_delete(reg) == VACCEL_OK);

	REQUIRE(vaccel_resource_release(&res) == VACCEL_OK);
	REQUIRE(vaccel_session_release(&sess) == VACCEL_OK);
	free(lib_path);
}

TEST_CASE("resource_registration_find", "[core][resource_registration]")
{
	char *lib_path = abs_path(BUILD_ROOT, "examples/libmytestlib.so");
	struct vaccel_session sess;
	struct vaccel_resource res;

	REQUIRE(vaccel_session_init(&sess, 0) == VACCEL_OK);
	REQUIRE(vaccel_resource_init(&res, lib_path, VACCEL_RESOURCE_LIB) ==
		VACCEL_OK);

	struct resource_registration *found_reg;

	SECTION("not linked")
	{
		found_reg = resource_registration_find(&res, &sess);
		REQUIRE(found_reg == nullptr);
	}

	struct resource_registration *reg;
	REQUIRE(resource_registration_new(&reg, &res, &sess) == VACCEL_OK);
	REQUIRE(resource_registration_link(reg) == VACCEL_OK);

	found_reg = resource_registration_find(&res, &sess);
	REQUIRE(found_reg == reg);

	SECTION("invalid arguments")
	{
		found_reg = resource_registration_find(nullptr, &sess);
		REQUIRE(found_reg == nullptr);

		found_reg = resource_registration_find(&res, nullptr);
		REQUIRE(found_reg == nullptr);
	}

	REQUIRE(resource_registration_unlink(reg) == VACCEL_OK);
	REQUIRE(resource_registration_delete(reg) == VACCEL_OK);

	REQUIRE(vaccel_resource_release(&res) == VACCEL_OK);
	REQUIRE(vaccel_session_release(&sess) == VACCEL_OK);
	free(lib_path);
}

TEST_CASE("resource_registration_find_and_unlink",
	  "[core][resource_registration]")
{
	char *lib_path = abs_path(BUILD_ROOT, "examples/libmytestlib.so");
	struct vaccel_session sess;
	struct vaccel_resource res;

	REQUIRE(vaccel_session_init(&sess, 0) == VACCEL_OK);
	REQUIRE(vaccel_resource_init(&res, lib_path, VACCEL_RESOURCE_LIB) ==
		VACCEL_OK);

	struct resource_registration *found_reg;

	SECTION("not linked")
	{
		found_reg = resource_registration_find_and_unlink(&res, &sess);
		REQUIRE(found_reg == nullptr);
	}

	struct resource_registration *reg;
	REQUIRE(resource_registration_new(&reg, &res, &sess) == VACCEL_OK);
	REQUIRE(resource_registration_link(reg) == VACCEL_OK);

	SECTION("invalid arguments")
	{
		found_reg =
			resource_registration_find_and_unlink(nullptr, &sess);
		REQUIRE(found_reg == nullptr);

		found_reg =
			resource_registration_find_and_unlink(&res, nullptr);
		REQUIRE(found_reg == nullptr);
	}

	found_reg = resource_registration_find_and_unlink(&res, &sess);
	REQUIRE(found_reg == reg);
	REQUIRE(list_empty(&res.sessions));
	REQUIRE(list_empty(&sess.resources[res.type]));
	REQUIRE(sess.resource_counts[res.type] == 0);
	REQUIRE(atomic_load(&res.refcount) == 0);

	REQUIRE(resource_registration_delete(reg) == VACCEL_OK);

	REQUIRE(vaccel_resource_release(&res) == VACCEL_OK);
	REQUIRE(vaccel_session_release(&sess) == VACCEL_OK);
	free(lib_path);
}

static auto foreach_callback(struct vaccel_resource *res,
			     struct vaccel_session *sess) -> int
{
	(void)sess;
	REQUIRE(res->type == VACCEL_RESOURCE_LIB);
	return VACCEL_OK;
}

TEST_CASE("resource_registration_foreach_session",
	  "[core][resource_registration]")
{
	int ret;
	char *lib_path = abs_path(BUILD_ROOT, "examples/libmytestlib.so");
	struct vaccel_session sess;
	struct vaccel_resource res;

	REQUIRE(vaccel_session_init(&sess, 0) == VACCEL_OK);
	REQUIRE(vaccel_resource_init(&res, lib_path, VACCEL_RESOURCE_LIB) ==
		VACCEL_OK);

	SECTION("no registrations")
	{
		ret = resource_registration_foreach_session(&res,
							    foreach_callback);
		REQUIRE(ret == VACCEL_OK);
	}

	struct resource_registration *reg;
	REQUIRE(resource_registration_new(&reg, &res, &sess) == VACCEL_OK);
	REQUIRE(resource_registration_link(reg) == VACCEL_OK);

	SECTION("invalid arguments")
	{
		ret = resource_registration_foreach_session(nullptr,
							    foreach_callback);
		REQUIRE(ret == VACCEL_EINVAL);

		ret = resource_registration_foreach_session(&res, nullptr);
		REQUIRE(ret == VACCEL_EINVAL);
	}

	ret = resource_registration_foreach_session(&res, foreach_callback);
	REQUIRE(ret == VACCEL_OK);

	REQUIRE(resource_registration_unlink(reg) == VACCEL_OK);
	REQUIRE(resource_registration_delete(reg) == VACCEL_OK);

	REQUIRE(vaccel_resource_release(&res) == VACCEL_OK);
	REQUIRE(vaccel_session_release(&sess) == VACCEL_OK);
	free(lib_path);
}

TEST_CASE("resource_registration_foreach_resource",
	  "[core][resource_registration]")
{
	int ret;
	char *lib_path = abs_path(BUILD_ROOT, "examples/libmytestlib.so");
	struct vaccel_session sess;
	struct vaccel_resource res;

	REQUIRE(vaccel_session_init(&sess, 0) == VACCEL_OK);
	REQUIRE(vaccel_resource_init(&res, lib_path, VACCEL_RESOURCE_LIB) ==
		VACCEL_OK);

	SECTION("no registrations")
	{
		ret = resource_registration_foreach_resource(&sess,
							     foreach_callback);
		REQUIRE(ret == VACCEL_OK);
	}

	struct resource_registration *reg;
	REQUIRE(resource_registration_new(&reg, &res, &sess) == VACCEL_OK);
	REQUIRE(resource_registration_link(reg) == VACCEL_OK);

	SECTION("invalid arguments")
	{
		ret = resource_registration_foreach_resource(nullptr,
							     foreach_callback);
		REQUIRE(ret == VACCEL_EINVAL);

		ret = resource_registration_foreach_resource(&sess, nullptr);
		REQUIRE(ret == VACCEL_EINVAL);
	}

	ret = resource_registration_foreach_resource(&sess, foreach_callback);
	REQUIRE(ret == VACCEL_OK);

	REQUIRE(resource_registration_unlink(reg) == VACCEL_OK);
	REQUIRE(resource_registration_delete(reg) == VACCEL_OK);

	REQUIRE(vaccel_resource_release(&res) == VACCEL_OK);
	REQUIRE(vaccel_session_release(&sess) == VACCEL_OK);
	free(lib_path);
}

enum {
	TEST_THREADS_NUM = 50,
	TEST_THREAD_REGS_NUM = 30,
	TEST_SESSIONS_NUM = 20,
	TEST_RESOURCES_NUM = 10
};

struct thread_data {
	size_t id;
	struct vaccel_resource *resource;
	struct vaccel_session *session;
};

static auto link_and_unlink_regs(void *arg) -> void *
{
	auto *data = (struct thread_data *)arg;
	struct vaccel_resource *res = data->resource;
	struct vaccel_session *sess = data->session;

	for (size_t i = 0; i < TEST_THREAD_REGS_NUM; i++) {
		struct resource_registration *reg;
		REQUIRE(resource_registration_new(&reg, res, sess) ==
			VACCEL_OK);
		REQUIRE(resource_registration_link(reg) == VACCEL_OK);
		printf("Thread %zu: Created registration for resource %" PRId64
		       ", session %" PRId64 "\n",
		       data->id, res->id, sess->id);

		// Add random delay to simulate work
		usleep(rand() % 1000);

		reg = resource_registration_find_and_unlink(res, sess);
		if (reg != nullptr) {
			REQUIRE(resource_registration_delete(reg) == VACCEL_OK);
			printf("Thread %zu: Deleted registration for resource %" PRId64
			       ", session %" PRId64 "\n",
			       data->id, res->id, sess->id);
		}
	}
	return nullptr;
}

static auto foreach_delete_callback(struct vaccel_resource *res,
				    struct vaccel_session *sess) -> int
{
	struct resource_registration *reg =
		resource_registration_find_and_unlink(res, sess);
	if (reg != nullptr) {
		REQUIRE(resource_registration_delete(reg) == VACCEL_OK);
		printf("Callback: Deleted registration for resource %" PRId64
		       ", session %" PRId64 "\n",
		       res->id, sess->id);
	}

	usleep(rand() % 1000);
	return VACCEL_OK;
}

static auto find_and_unlink_res_regs(void *arg) -> void *
{
	auto *data = (struct thread_data *)arg;

	int const ret = resource_registration_foreach_session(
		data->resource, foreach_delete_callback);
	REQUIRE(ret == VACCEL_OK);
	return nullptr;
}

static auto find_and_unlink_sess_regs(void *arg) -> void *
{
	auto *data = (struct thread_data *)arg;

	int const ret = resource_registration_foreach_resource(
		data->session, foreach_delete_callback);
	REQUIRE(ret == VACCEL_OK);
	return nullptr;
}

TEST_CASE("resource_registration_link_and_unlink_concurrent",
	  "[core][resource_registration]")
{
	char *lib_path = abs_path(BUILD_ROOT, "examples/libmytestlib.so");
	struct vaccel_resource resources[TEST_RESOURCES_NUM];
	struct vaccel_session sessions[TEST_SESSIONS_NUM];

	for (auto &res : resources)
		REQUIRE(vaccel_resource_init(&res, lib_path,
					     VACCEL_RESOURCE_LIB) == VACCEL_OK);

	for (auto &sess : sessions)
		REQUIRE(vaccel_session_init(&sess, 0) == VACCEL_OK);

	pthread_t threads[TEST_THREADS_NUM];
	struct thread_data thread_data[TEST_THREADS_NUM];

	size_t nr_resources = 0;
	size_t nr_sessions = 0;
	for (size_t i = 0; i < TEST_THREADS_NUM - 2; i++) {
		if (nr_resources >= TEST_RESOURCES_NUM)
			nr_resources = 0;
		if (nr_sessions >= TEST_SESSIONS_NUM)
			nr_sessions = 0;
		thread_data[i].id = i;
		thread_data[i].resource = &resources[nr_resources++];
		thread_data[i].session = &sessions[nr_sessions++];
		pthread_create(&threads[i], nullptr, link_and_unlink_regs,
			       &thread_data[i]);
	}

	thread_data[TEST_THREADS_NUM - 2].id = TEST_THREADS_NUM - 2;
	thread_data[TEST_THREADS_NUM - 2].resource = &resources[0];
	thread_data[TEST_THREADS_NUM - 2].session = &sessions[0];
	pthread_create(&threads[TEST_THREADS_NUM - 2], nullptr,
		       find_and_unlink_res_regs,
		       &thread_data[TEST_THREADS_NUM - 2]);

	thread_data[TEST_THREADS_NUM - 1].id = TEST_THREADS_NUM - 1;
	thread_data[TEST_THREADS_NUM - 1].resource = &resources[1];
	thread_data[TEST_THREADS_NUM - 1].session = &sessions[1];
	pthread_create(&threads[TEST_THREADS_NUM - 1], nullptr,
		       find_and_unlink_sess_regs,
		       &thread_data[TEST_THREADS_NUM - 1]);

	for (unsigned long const thread : threads)
		pthread_join(thread, nullptr);

	for (auto &sess : sessions)
		REQUIRE(vaccel_session_release(&sess) == VACCEL_OK);

	for (auto &res : resources)
		REQUIRE(vaccel_resource_release(&res) == VACCEL_OK);

	free(lib_path);
}
