// SPDX-License-Identifier: Apache-2.0

/*
 * The code below performs unit testing to sessions.
 *
 * 1) sessions_bootstrap()
 * 2) vaccel_session_init()
 * 3) vaccel_session_update()
 * 4) vaccel_session_free()
 * 5) session_register_resource()
 * 6) session_unregister_resource()
 * 7) vaccel_session_has_resource)()
 * 8) session_cleanup()
 *
 */

#include <catch.hpp>
#include <fff.h>
#include <utils.hpp>

DEFINE_FFF_GLOBALS;

#include <vaccel.h>

extern "C" {
FAKE_VALUE_FUNC(struct vaccel_plugin *, get_virtio_plugin);
FAKE_VALUE_FUNC(struct vaccel_session *, sess_free);
}

enum { MAX_VACCEL_SESSIONS = 1024 };

// Mock functions for session initialization and cleanup
auto mock_sess_init(struct vaccel_session *sess, uint32_t flags) -> int
{
	(void)sess;
	(void)flags;
	return 0;
}

auto mock_sess_update(struct vaccel_session *sess, uint32_t flags) -> int
{
	(void)sess;
	(void)flags;
	return 0;
}

auto mock_sess_free(struct vaccel_session *sess) -> int
{
	(void)sess;
	return 0;
}

auto mock_resource_register(struct vaccel_resource *res,
			    struct vaccel_session *sess) -> int
{
	res->remote_id = 1;
	(void)sess;
	return 0;
}

auto mock_resource_unregister(struct vaccel_resource *res,
			      struct vaccel_session *sess) -> int
{
	(void)res;
	(void)sess;
	return 0;
}

auto mock_resource_new(vaccel_resource_t type, void *data,
		       vaccel_id_t *id) -> int
{
	(void)type;
	(void)data;
	*id = 1;
	return 0;
}

auto mock_resource_destroy(vaccel_id_t id) -> int
{
	(void)id;
	return 0;
}

// Test case for session initialization
TEST_CASE("session_init", "[session]")
{
	int ret;

	RESET_FAKE(get_virtio_plugin);

	struct vaccel_session sess;
	sess.hint = 0;
	sess.session_id = 0;
	sess.resources = nullptr;
	sess.priv = nullptr;

	// Test handling of null session
	ret = vaccel_session_init(nullptr, 1);
	REQUIRE(ret == VACCEL_EINVAL);
	REQUIRE(sess.hint == 0);
	REQUIRE(sess.session_id == 0);
	REQUIRE(sess.resources == nullptr);
	REQUIRE(sess.priv == nullptr);

	// Test session initialization and cleanup
	ret = vaccel_session_init(&sess, 1);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(sess.session_id);
	REQUIRE(sess.hint == 1);
	REQUIRE(sess.resources);
	REQUIRE(sess.priv == nullptr);

	ret = vaccel_session_free(nullptr);
	REQUIRE(ret == VACCEL_EINVAL);
	REQUIRE(sess.hint == 1);
	REQUIRE(sess.session_id);
	REQUIRE(sess.resources);
	REQUIRE(sess.priv == nullptr);

	REQUIRE(vaccel_session_free(&sess) == VACCEL_OK);
	REQUIRE(sess.session_id);
	REQUIRE(sess.hint == 1);
	REQUIRE(sess.resources == nullptr);
	REQUIRE(sess.priv == nullptr);
}

// Test case for session update and cleanup
TEST_CASE("session_update_and_free", "[session]")
{
	struct vaccel_session sess;
	sess.session_id = 0;
	sess.resources = nullptr;
	sess.priv = nullptr;
	int ret;

	RESET_FAKE(get_virtio_plugin);

	ret = vaccel_session_init(&sess, 1);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(sess.session_id);
	REQUIRE(sess.hint == 1);
	REQUIRE(sess.resources);
	REQUIRE(sess.priv == nullptr);

	// Test session update
	ret = vaccel_session_update(&sess, 2);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(sess.session_id);
	REQUIRE(sess.hint == 2);
	REQUIRE(sess.resources);
	REQUIRE(sess.priv == nullptr);

	ret = vaccel_session_update(nullptr, 2);
	REQUIRE(ret == VACCEL_EINVAL);
	REQUIRE(sess.session_id);
	REQUIRE(sess.hint == 2);
	REQUIRE(sess.resources);
	REQUIRE(sess.priv == nullptr);

	ret = vaccel_session_free(nullptr);
	REQUIRE(ret == EINVAL);
	REQUIRE(sess.session_id);
	REQUIRE(sess.hint == 2);
	REQUIRE(sess.resources);
	REQUIRE(sess.priv == nullptr);

	// Test session cleanup
	REQUIRE(vaccel_session_free(&sess) == VACCEL_OK);
	REQUIRE(sess.session_id);
	REQUIRE(sess.hint == 2);
	REQUIRE(sess.resources == nullptr);
	REQUIRE(sess.priv == nullptr);
}

// Test case for unregistering a session with null parameters
TEST_CASE("session_unregister_null", "[session]")
{
	int ret;

	RESET_FAKE(get_virtio_plugin);

	struct vaccel_session sess;
	sess.session_id = 0;
	sess.resources = nullptr;
	sess.priv = nullptr;

	ret = vaccel_session_init(&sess, 1);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(sess.session_id);
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

	REQUIRE(sess.session_id);
	REQUIRE(sess.hint == 1);
	REQUIRE(sess.resources);
	REQUIRE(sess.priv == nullptr);

	ret = session_register_resource(&sess, &res);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(sess.session_id);
	REQUIRE(sess.hint == 1);
	REQUIRE_FALSE(list_empty(&sess.resources->registered[res.type]));
	REQUIRE(sess.priv == nullptr);

	bool check_bool = vaccel_session_has_resource(&sess, &res);
	REQUIRE(check_bool);

	ret = session_unregister_resource(nullptr, &res);
	REQUIRE(ret == VACCEL_EINVAL);
	ret = session_unregister_resource(&sess, nullptr);
	REQUIRE(ret == VACCEL_EINVAL);

	REQUIRE(sess.session_id);
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
	REQUIRE(sess.session_id);
	REQUIRE(sess.hint == 1);
	REQUIRE(list_empty(&sess.resources->registered[res.type]));
	REQUIRE(sess.priv == nullptr);

	check_bool = vaccel_session_has_resource(&sess, &res);
	REQUIRE(!check_bool);

	ret = vaccel_session_free(&sess);
	REQUIRE(ret == VACCEL_OK);

	REQUIRE(sess.session_id);
	REQUIRE(sess.hint == 1);
	REQUIRE(sess.resources == nullptr);
	REQUIRE(sess.priv == nullptr);
}

// Test case for session initialization, update, registration, and cleanup
TEST_CASE("session_ops", "[session]")
{
	int ret;
	struct vaccel_session test_sess;
	test_sess.hint = 0;
	test_sess.session_id = 0;
	test_sess.resources = nullptr;
	test_sess.priv = nullptr;
	struct vaccel_resource test_res;
	test_res.type = VACCEL_RESOURCE_LIB;
	test_res.id = 1;

	RESET_FAKE(get_virtio_plugin);

	ret = vaccel_session_init(&test_sess, 1);
	REQUIRE(VACCEL_OK == ret);

	REQUIRE(ret == VACCEL_OK);
	REQUIRE(test_sess.session_id);
	REQUIRE(test_sess.hint == 1);
	REQUIRE(test_sess.resources);
	REQUIRE(test_sess.priv == nullptr);

	ret = vaccel_session_update(&test_sess, 2);
	REQUIRE(VACCEL_OK == ret);
	REQUIRE(test_sess.session_id);
	REQUIRE(test_sess.hint == 2);
	REQUIRE(test_sess.resources);
	REQUIRE(test_sess.priv == nullptr);

	ret = session_register_resource(&test_sess, &test_res);
	REQUIRE(VACCEL_OK == ret);
	REQUIRE(test_sess.session_id);
	REQUIRE(test_sess.hint == 2);
	REQUIRE_FALSE(
		list_empty(&test_sess.resources->registered[test_res.type]));
	REQUIRE(test_sess.priv == nullptr);

	ret = session_unregister_resource(&test_sess, &test_res);
	REQUIRE(VACCEL_OK == ret);
	REQUIRE(test_sess.session_id);
	REQUIRE(test_sess.hint == 2);
	REQUIRE(list_empty(&test_sess.resources->registered[test_res.type]));
	REQUIRE(test_sess.priv == nullptr);

	ret = vaccel_session_free(&test_sess);
	REQUIRE(VACCEL_OK == ret);
	REQUIRE(test_sess.session_id);
	REQUIRE(test_sess.hint == 2);
	REQUIRE(test_sess.resources == nullptr);
	REQUIRE(test_sess.priv == nullptr);
}

// Test case for session initialization, update, registration, and cleanup with
// a VirtIO plugin
TEST_CASE("session_virtio", "[session]")
{
	int ret;
	struct vaccel_session test_sess;
	test_sess.hint = 0;
	test_sess.session_id = 1;
	test_sess.resources = nullptr;
	test_sess.priv = nullptr;
	struct vaccel_resource test_res;
	test_res.type = VACCEL_RESOURCE_LIB;
	test_res.id = 1;
	test_res.remote_id = -1;

	RESET_FAKE(get_virtio_plugin);

	// Create a mock plugin
	struct vaccel_plugin_info v_mock_info;
	v_mock_info.name = "fake_virtio";
	v_mock_info.sess_init = mock_sess_init;
	v_mock_info.sess_free = mock_sess_free;
	v_mock_info.sess_update = mock_sess_update;
	v_mock_info.resource_register = mock_resource_register;
	v_mock_info.resource_unregister = mock_resource_unregister;

	struct vaccel_plugin v_mock;
	v_mock.info = &v_mock_info;

	get_virtio_plugin_fake.return_val = &v_mock;

	ret = vaccel_session_init(&test_sess, 1 | VACCEL_REMOTE);
	REQUIRE(VACCEL_OK == ret);

	ret = vaccel_session_update(&test_sess, 2);
	REQUIRE(VACCEL_OK == ret);

	ret = session_register_resource(&test_sess, &test_res);
	REQUIRE(VACCEL_OK == ret);

	ret = session_unregister_resource(&test_sess, &test_res);
	REQUIRE(VACCEL_OK == ret);

	ret = vaccel_session_free(&test_sess);
	REQUIRE(VACCEL_OK == ret);

	// Ensure that the VirtIO plugin was called the expected number of times
	// Note: session_*register_resource() does not call VirtIO functions
	REQUIRE(get_virtio_plugin_fake.call_count == 3);
}
