// SPDX-License-Identifier: Apache-2.0

/*
 * The code below performs unit testing to sessions.
 *
 * 1) sessions_bootstrap()
 * 2) vaccel_session_init()
 * 3) vaccel_session_update()
 * 4) vaccel_session_release()
 * 5) session_register_resource()
 * 6) session_unregister_resource()
 * 7) vaccel_session_has_resource)()
 * 8) session_cleanup()
 * 9) vaccel_session_resource_by_type()
 * 10) vaccel_session_resource_by_id()
 * 11) vaccel_session_resources_by_type()
 *
 */

#include "vaccel.h"
#include "utils.hpp"
#include <catch.hpp>
#include <cerrno>
#include <cstdlib>
#include <fff.h>
#include <mock_virtio.hpp>

DEFINE_FFF_GLOBALS;

extern "C" {
FAKE_VALUE_FUNC(struct vaccel_plugin *, plugin_virtio);
}

enum { MAX_VACCEL_SESSIONS = 1024 };

// Test case for session initialization
TEST_CASE("session_init", "[core][session]")
{
	int ret;

	RESET_FAKE(plugin_virtio);

	struct vaccel_session sess;
	sess.hint = 0;
	sess.id = 0;
	sess.resources = nullptr;
	sess.priv = nullptr;

	// Test handling of null session
	ret = vaccel_session_init(nullptr, 1);
	REQUIRE(ret == VACCEL_EINVAL);
	REQUIRE(sess.hint == 0);
	REQUIRE(sess.id == 0);
	REQUIRE(sess.resources == nullptr);
	REQUIRE(sess.priv == nullptr);

	// Test session initialization and cleanup
	ret = vaccel_session_init(&sess, 1);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(sess.id);
	REQUIRE(sess.hint == 1);
	REQUIRE(sess.resources);
	REQUIRE(sess.priv == nullptr);

	ret = vaccel_session_release(nullptr);
	REQUIRE(ret == VACCEL_EINVAL);
	REQUIRE(sess.hint == 1);
	REQUIRE(sess.id);
	REQUIRE(sess.resources);
	REQUIRE(sess.priv == nullptr);

	REQUIRE(vaccel_session_release(&sess) == VACCEL_OK);
	REQUIRE(sess.id);
	REQUIRE(sess.hint == 1);
	REQUIRE(sess.resources == nullptr);
	REQUIRE(sess.priv == nullptr);
}

// Test case for session update and cleanup
TEST_CASE("session_update_and_free", "[core][session]")
{
	struct vaccel_session sess;
	sess.id = 0;
	sess.resources = nullptr;
	sess.priv = nullptr;
	int ret;

	RESET_FAKE(plugin_virtio);

	ret = vaccel_session_init(&sess, 1);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(sess.id);
	REQUIRE(sess.hint == 1);
	REQUIRE(sess.resources);
	REQUIRE(sess.priv == nullptr);

	// Test session update
	ret = vaccel_session_update(&sess, 2);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(sess.id);
	REQUIRE(sess.hint == 2);
	REQUIRE(sess.resources);
	REQUIRE(sess.priv == nullptr);

	ret = vaccel_session_update(nullptr, 2);
	REQUIRE(ret == VACCEL_EINVAL);
	REQUIRE(sess.id);
	REQUIRE(sess.hint == 2);
	REQUIRE(sess.resources);
	REQUIRE(sess.priv == nullptr);

	ret = vaccel_session_release(nullptr);
	REQUIRE(ret == EINVAL);
	REQUIRE(sess.id);
	REQUIRE(sess.hint == 2);
	REQUIRE(sess.resources);
	REQUIRE(sess.priv == nullptr);

	// Test session cleanup
	REQUIRE(vaccel_session_release(&sess) == VACCEL_OK);
	REQUIRE(sess.id);
	REQUIRE(sess.hint == 2);
	REQUIRE(sess.resources == nullptr);
	REQUIRE(sess.priv == nullptr);
}

// Test case for unregistering a session with null parameters
TEST_CASE("session_unregister_null", "[core][session]")
{
	int ret;

	RESET_FAKE(plugin_virtio);

	struct vaccel_session sess;
	sess.id = 0;
	sess.resources = nullptr;
	sess.priv = nullptr;

	ret = vaccel_session_init(&sess, 1);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(sess.id);
	REQUIRE(sess.hint == 1);
	REQUIRE(sess.resources);
	REQUIRE(sess.priv == nullptr);

	struct vaccel_resource res;
	res.type = VACCEL_RESOURCE_LIB;
	res.id = 1;

	ret = session_register_resource(nullptr, nullptr);
	REQUIRE(ret == VACCEL_EINVAL);
	ret = session_register_resource(nullptr, &res);
	REQUIRE(ret == VACCEL_EINVAL);
	ret = session_register_resource(&sess, nullptr);
	REQUIRE(ret == VACCEL_EINVAL);

	REQUIRE(sess.id);
	REQUIRE(sess.hint == 1);
	REQUIRE(sess.resources);
	REQUIRE(sess.priv == nullptr);

	ret = session_register_resource(&sess, &res);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(sess.id);
	REQUIRE(sess.hint == 1);
	REQUIRE_FALSE(list_empty(&sess.resources->registered[res.type]));
	REQUIRE(sess.priv == nullptr);

	bool check_bool = vaccel_session_has_resource(&sess, &res);
	REQUIRE(check_bool);

	ret = session_unregister_resource(nullptr, &res);
	REQUIRE(ret == VACCEL_EINVAL);
	ret = session_unregister_resource(&sess, nullptr);
	REQUIRE(ret == VACCEL_EINVAL);

	REQUIRE(sess.id);
	REQUIRE(sess.hint == 1);
	REQUIRE_FALSE(list_empty(&sess.resources->registered[res.type]));
	REQUIRE(sess.priv == nullptr);

	res.type = VACCEL_RESOURCE_MAX;
	res.id = 2;
	ret = session_unregister_resource(&sess, &res);
	REQUIRE(ret == VACCEL_EINVAL);

	res.type = VACCEL_RESOURCE_LIB;
	res.id = 1;
	ret = session_unregister_resource(&sess, &res);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(sess.id);
	REQUIRE(sess.hint == 1);
	REQUIRE(list_empty(&sess.resources->registered[res.type]));
	REQUIRE(sess.priv == nullptr);

	check_bool = vaccel_session_has_resource(&sess, &res);
	REQUIRE(!check_bool);

	ret = vaccel_session_release(&sess);
	REQUIRE(ret == VACCEL_OK);

	REQUIRE(sess.id);
	REQUIRE(sess.hint == 1);
	REQUIRE(sess.resources == nullptr);
	REQUIRE(sess.priv == nullptr);
}

// Test case for session initialization, update, registration, and cleanup
TEST_CASE("session_ops", "[core][session]")
{
	int ret;
	struct vaccel_session test_sess;
	test_sess.hint = 0;
	test_sess.id = 0;
	test_sess.resources = nullptr;
	test_sess.priv = nullptr;
	struct vaccel_resource test_res;
	test_res.type = VACCEL_RESOURCE_LIB;
	test_res.id = 1;

	RESET_FAKE(plugin_virtio);

	ret = vaccel_session_init(&test_sess, 1);
	REQUIRE(VACCEL_OK == ret);

	REQUIRE(ret == VACCEL_OK);
	REQUIRE(test_sess.id);
	REQUIRE(test_sess.hint == 1);
	REQUIRE(test_sess.resources);
	REQUIRE(test_sess.priv == nullptr);

	ret = vaccel_session_update(&test_sess, 2);
	REQUIRE(VACCEL_OK == ret);
	REQUIRE(test_sess.id);
	REQUIRE(test_sess.hint == 2);
	REQUIRE(test_sess.resources);
	REQUIRE(test_sess.priv == nullptr);

	ret = session_register_resource(&test_sess, &test_res);
	REQUIRE(VACCEL_OK == ret);
	REQUIRE(test_sess.id);
	REQUIRE(test_sess.hint == 2);
	REQUIRE_FALSE(
		list_empty(&test_sess.resources->registered[test_res.type]));
	REQUIRE(test_sess.priv == nullptr);

	ret = session_unregister_resource(&test_sess, &test_res);
	REQUIRE(VACCEL_OK == ret);
	REQUIRE(test_sess.id);
	REQUIRE(test_sess.hint == 2);
	REQUIRE(list_empty(&test_sess.resources->registered[test_res.type]));
	REQUIRE(test_sess.priv == nullptr);

	ret = vaccel_session_release(&test_sess);
	REQUIRE(VACCEL_OK == ret);
	REQUIRE(test_sess.id);
	REQUIRE(test_sess.hint == 2);
	REQUIRE(test_sess.resources == nullptr);
	REQUIRE(test_sess.priv == nullptr);
}

// Test case for session initialization, update, registration, and cleanup with
// a VirtIO plugin
TEST_CASE("session_virtio", "[core][session]")
{
	int ret;
	struct vaccel_session test_sess;
	test_sess.hint = 0;
	test_sess.id = 1;
	test_sess.resources = nullptr;
	test_sess.priv = nullptr;
	struct vaccel_resource test_res;
	test_res.type = VACCEL_RESOURCE_LIB;
	test_res.id = 1;
	test_res.remote_id = -1;

	RESET_FAKE(plugin_virtio);

	plugin_virtio_fake.custom_fake = mock_virtio_plugin_virtio;

	ret = vaccel_session_init(&test_sess, 1 | VACCEL_PLUGIN_REMOTE);
	REQUIRE(VACCEL_OK == ret);

	ret = vaccel_session_update(&test_sess, 2);
	REQUIRE(VACCEL_OK == ret);

	ret = session_register_resource(&test_sess, &test_res);
	REQUIRE(VACCEL_OK == ret);

	ret = session_unregister_resource(&test_sess, &test_res);
	REQUIRE(VACCEL_OK == ret);

	ret = vaccel_session_release(&test_sess);
	REQUIRE(VACCEL_OK == ret);

	// Ensure that the VirtIO plugin was called the expected number of times
	// Note: session_*register_resource() does not call VirtIO functions
	REQUIRE(plugin_virtio_fake.call_count == 3);
}

// Test case for finding a registered resource by type
TEST_CASE("vaccel_session_resource_by_type", "[core][session]")
{
	struct vaccel_resource *res_ptr;
	struct vaccel_session sess;
	int ret;

	/* Invalid input */
	ret = vaccel_session_resource_by_type(nullptr, &res_ptr,
					      VACCEL_RESOURCE_LIB);
	REQUIRE(ret == VACCEL_EINVAL);

	ret = vaccel_session_resource_by_type(&sess, nullptr,
					      VACCEL_RESOURCE_LIB);
	REQUIRE(ret == VACCEL_EINVAL);

	ret = vaccel_session_resource_by_type(&sess, &res_ptr,
					      VACCEL_RESOURCE_MAX);
	REQUIRE(ret == VACCEL_EINVAL);

	/* Failing because there's no resource */
	ret = vaccel_session_init(&sess, 0);
	REQUIRE(ret == VACCEL_OK);

	ret = vaccel_session_resource_by_type(&sess, &res_ptr,
					      VACCEL_RESOURCE_LIB);
	REQUIRE(ret == VACCEL_ENOENT);

	ret = vaccel_session_resource_by_type(&sess, &res_ptr,
					      VACCEL_RESOURCE_MODEL);
	REQUIRE(ret == VACCEL_ENOENT);

	ret = vaccel_session_resource_by_type(&sess, &res_ptr,
					      VACCEL_RESOURCE_DATA);
	REQUIRE(ret == VACCEL_ENOENT);

	/* Failing because the resource is not registered */
	struct vaccel_resource res;
	char *lib_path = abs_path(BUILD_ROOT, "examples/libmytestlib.so");
	ret = vaccel_resource_init(&res, lib_path, VACCEL_RESOURCE_LIB);
	REQUIRE(ret == VACCEL_OK);

	ret = vaccel_session_resource_by_type(&sess, &res_ptr,
					      VACCEL_RESOURCE_LIB);
	REQUIRE(ret == VACCEL_ENOENT);

	/* Success */
	ret = vaccel_resource_register(&res, &sess);
	REQUIRE(ret == VACCEL_OK);

	ret = vaccel_session_resource_by_type(&sess, &res_ptr,
					      VACCEL_RESOURCE_LIB);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(res_ptr == &res);

	/* Failing because of wrong type, although registered */
	ret = vaccel_session_resource_by_type(&sess, &res_ptr,
					      VACCEL_RESOURCE_MODEL);
	REQUIRE(ret == VACCEL_ENOENT);

	ret = vaccel_session_resource_by_type(&sess, &res_ptr,
					      VACCEL_RESOURCE_DATA);
	REQUIRE(ret == VACCEL_ENOENT);

	/* Failing because the resource was unregistered */
	ret = vaccel_resource_unregister(&res, &sess);
	REQUIRE(ret == VACCEL_OK);

	ret = vaccel_session_resource_by_type(&sess, &res_ptr,
					      VACCEL_RESOURCE_LIB);
	REQUIRE(ret == VACCEL_ENOENT);

	/* Close */
	ret = vaccel_resource_release(&res);
	REQUIRE(ret == VACCEL_OK);

	ret = vaccel_session_release(&sess);
	REQUIRE(ret == VACCEL_OK);

	free(lib_path);
}

TEST_CASE("vaccel_session_resource_by_id", "[core][session]")
{
	struct vaccel_resource *res_ptr;
	struct vaccel_session sess;
	int ret;

	/* Invalid input */
	ret = vaccel_session_resource_by_id(nullptr, &res_ptr, 1);
	REQUIRE(ret == VACCEL_EINVAL);

	ret = vaccel_session_resource_by_id(&sess, nullptr, 1);
	REQUIRE(ret == VACCEL_EINVAL);

	ret = vaccel_session_resource_by_id(&sess, &res_ptr, 0);
	REQUIRE(ret == VACCEL_EINVAL);

	ret = vaccel_session_resource_by_id(&sess, &res_ptr, -1);
	REQUIRE(ret == VACCEL_EINVAL);

	/* Failing because there's no resource */
	ret = vaccel_session_init(&sess, 0);
	REQUIRE(ret == VACCEL_OK);

	ret = vaccel_session_resource_by_id(&sess, &res_ptr, 1);
	REQUIRE(ret == VACCEL_ENOENT);

	/* Failing because the resource is not registered */
	struct vaccel_resource res;
	char *lib_path = abs_path(BUILD_ROOT, "examples/libmytestlib.so");
	ret = vaccel_resource_init(&res, lib_path, VACCEL_RESOURCE_LIB);
	REQUIRE(ret == VACCEL_OK);

	ret = vaccel_session_resource_by_id(&sess, &res_ptr, res.id);
	REQUIRE(ret == VACCEL_ENOENT);

	/* Success */
	ret = vaccel_resource_register(&res, &sess);
	REQUIRE(ret == VACCEL_OK);

	ret = vaccel_session_resource_by_id(&sess, &res_ptr, res.id);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(res_ptr == &res);

	/* Failing because of wrong id */
	ret = vaccel_session_resource_by_id(&sess, &res_ptr, res.id + 1);
	REQUIRE(ret == VACCEL_ENOENT);

	/* Failing because the resource was unregistered */
	ret = vaccel_resource_unregister(&res, &sess);
	REQUIRE(ret == VACCEL_OK);

	ret = vaccel_session_resource_by_id(&sess, &res_ptr, res.id);
	REQUIRE(ret == VACCEL_ENOENT);

	/* Close */
	ret = vaccel_resource_release(&res);
	REQUIRE(ret == VACCEL_OK);

	ret = vaccel_session_release(&sess);
	REQUIRE(ret == VACCEL_OK);

	free(lib_path);
}

TEST_CASE("vaccel_session_resources_by_type", "[core][session]")
{
	struct vaccel_resource **res_ptr;
	struct vaccel_session sess;
	size_t nr_found;
	int ret;

	/* Invalid input */
	ret = vaccel_session_resources_by_type(nullptr, &res_ptr, &nr_found,
					       VACCEL_RESOURCE_LIB);
	REQUIRE(ret == VACCEL_EINVAL);

	ret = vaccel_session_resources_by_type(&sess, nullptr, &nr_found,
					       VACCEL_RESOURCE_LIB);
	REQUIRE(ret == VACCEL_EINVAL);

	ret = vaccel_session_resources_by_type(&sess, &res_ptr, nullptr,
					       VACCEL_RESOURCE_LIB);
	REQUIRE(ret == VACCEL_EINVAL);

	ret = vaccel_session_resources_by_type(&sess, &res_ptr, &nr_found,
					       VACCEL_RESOURCE_MAX);
	REQUIRE(ret == VACCEL_EINVAL);

	/* Failing because there's no resource */
	ret = vaccel_session_init(&sess, 0);
	REQUIRE(ret == VACCEL_OK);

	ret = vaccel_session_resources_by_type(&sess, &res_ptr, &nr_found,
					       VACCEL_RESOURCE_LIB);
	REQUIRE(ret == VACCEL_ENOENT);
	REQUIRE(nr_found == 0);

	ret = vaccel_session_resources_by_type(&sess, &res_ptr, &nr_found,
					       VACCEL_RESOURCE_MODEL);
	REQUIRE(ret == VACCEL_ENOENT);
	REQUIRE(nr_found == 0);

	ret = vaccel_session_resources_by_type(&sess, &res_ptr, &nr_found,
					       VACCEL_RESOURCE_DATA);
	REQUIRE(ret == VACCEL_ENOENT);
	REQUIRE(nr_found == 0);

	/* Failing because the resource is not registered */
	struct vaccel_resource res;
	char *lib_path = abs_path(BUILD_ROOT, "examples/libmytestlib.so");
	ret = vaccel_resource_init(&res, lib_path, VACCEL_RESOURCE_LIB);
	REQUIRE(ret == VACCEL_OK);

	ret = vaccel_session_resources_by_type(&sess, &res_ptr, &nr_found,
					       VACCEL_RESOURCE_LIB);
	REQUIRE(ret == VACCEL_ENOENT);
	REQUIRE(nr_found == 0);

	/* Success */
	ret = vaccel_resource_register(&res, &sess);
	REQUIRE(ret == VACCEL_OK);

	ret = vaccel_session_resources_by_type(&sess, &res_ptr, &nr_found,
					       VACCEL_RESOURCE_LIB);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(nr_found == 1);
	REQUIRE(res_ptr[0] == &res);

	free(res_ptr);

	/* Success with more than 1 resource */
	struct vaccel_resource res2;
	ret = vaccel_resource_init(&res2, lib_path, VACCEL_RESOURCE_LIB);
	REQUIRE(ret == VACCEL_OK);

	ret = vaccel_resource_register(&res2, &sess);
	REQUIRE(ret == VACCEL_OK);

	ret = vaccel_session_resources_by_type(&sess, &res_ptr, &nr_found,
					       VACCEL_RESOURCE_LIB);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(nr_found == 2);
	REQUIRE(res_ptr[0] == &res);
	REQUIRE(res_ptr[1] == &res2);

	free(res_ptr);

	/* Delete the second resource */
	ret = vaccel_resource_unregister(&res2, &sess);
	REQUIRE(ret == VACCEL_OK);

	ret = vaccel_resource_release(&res2);
	REQUIRE(ret == VACCEL_OK);

	/* Failing because of wrong type, although registered */
	ret = vaccel_session_resources_by_type(&sess, &res_ptr, &nr_found,
					       VACCEL_RESOURCE_MODEL);
	REQUIRE(ret == VACCEL_ENOENT);

	ret = vaccel_session_resources_by_type(&sess, &res_ptr, &nr_found,
					       VACCEL_RESOURCE_DATA);
	REQUIRE(ret == VACCEL_ENOENT);

	/* Failing because the resource was unregistered */
	ret = vaccel_resource_unregister(&res, &sess);
	REQUIRE(ret == VACCEL_OK);

	ret = vaccel_session_resources_by_type(&sess, &res_ptr, &nr_found,
					       VACCEL_RESOURCE_LIB);
	REQUIRE(ret == VACCEL_ENOENT);

	/* Close */
	ret = vaccel_resource_release(&res);
	REQUIRE(ret == VACCEL_OK);

	ret = vaccel_session_release(&sess);
	REQUIRE(ret == VACCEL_OK);

	free(lib_path);
}
