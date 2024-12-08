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
 *
 */

#include "vaccel.h"
#include <catch.hpp>
#include <cerrno>
#include <fff.h>
#include <mock_virtio.hpp>

DEFINE_FFF_GLOBALS;

extern "C" {
FAKE_VALUE_FUNC(struct vaccel_plugin *, get_virtio_plugin);
}

enum { MAX_VACCEL_SESSIONS = 1024 };

// Test case for session initialization
TEST_CASE("session_init", "[core][session]")
{
	int ret;

	RESET_FAKE(get_virtio_plugin);

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

	RESET_FAKE(get_virtio_plugin);

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

	RESET_FAKE(get_virtio_plugin);

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

	RESET_FAKE(get_virtio_plugin);

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

	RESET_FAKE(get_virtio_plugin);

	get_virtio_plugin_fake.custom_fake = mock_virtio_get_virtio_plugin;

	ret = vaccel_session_init(&test_sess, 1 | VACCEL_REMOTE);
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
	REQUIRE(get_virtio_plugin_fake.call_count == 3);
}
