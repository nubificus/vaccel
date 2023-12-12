/*
 * The code below performs unit testing to sessions.
 *
 * 1) sessions_bootstrap()
 * 2) vaccel_sess_init()
 * 3) vaccel_sess_update()
 * 4) vaccel_sess_free()
 * 5) vaccel_sess_unregister()
 * 6) vaccel_sess_register()
 * 7) vaccel_sess_has_resource)()
 * 8) session_cleanup()
 *
 */
 
#include "fff.h"
#include <catch.hpp>

#include <atomic>

using atomic_int = std::atomic<int>;
using atomic_uint = std::atomic<unsigned int>;

DEFINE_FFF_GLOBALS;
extern "C" {
#include "id_pool.h"
#include "log.h"
#include "plugin.h"
#include "resources.h"
#include "utils.h"
#include "session.h"
FAKE_VALUE_FUNC(struct vaccel_plugin*, get_virtio_plugin);
FAKE_VALUE_FUNC(struct vaccel_session*, sess_free);
}


#define MAX_VACCEL_SESSIONS 1024

// Mock functions for session initialization and cleanup
int mock_sess_init(vaccel_session* sess, uint32_t flags)
{
    (void)sess;
    (void)flags;
    return 0;
}

int mock_sess_update(vaccel_session* sess, uint32_t flags)
{
    (void)sess;
    (void)flags;
    return 0;
}

int mock_sess_free(vaccel_session* sess)
{
    (void)sess;
    return 0;
}

int mock_sess_register(uint32_t sess_id, vaccel_id_t resource_id)
{
    (void)sess_id;
    (void)resource_id;
    return 0;
}

int mock_sess_unregister(uint32_t sess_id, vaccel_id_t resource_id)
{
    (void)sess_id;
    (void)resource_id;
    return 0;
}

// Test case for session initialization
TEST_CASE("session_init", "[session]")
{

    int ret;

    // Ensure that the session system is initialized
    ret = sessions_bootstrap();
    REQUIRE(ret == VACCEL_OK);
    struct vaccel_session sess;
    sess.hint = 0;
    sess.session_id = 0;
    sess.resources = nullptr;
    sess.priv = nullptr;

    // Test handling of null session
    ret = vaccel_sess_init(NULL, 1);
    REQUIRE(ret == VACCEL_EINVAL);
    REQUIRE(sess.hint == 0);
    REQUIRE(sess.session_id == 0);
    REQUIRE(sess.resources == nullptr);
    REQUIRE(sess.priv == nullptr);

    // Test session initialization and cleanup
    ret = vaccel_sess_init(&sess, 1);
    REQUIRE(ret == VACCEL_OK);
    REQUIRE(sess.session_id);
    REQUIRE(sess.hint == 1);
    REQUIRE(sess.resources);
    REQUIRE(sess.priv == nullptr);

    ret = vaccel_sess_free(NULL);
    REQUIRE(ret == VACCEL_EINVAL);
    REQUIRE(sess.hint == 1);
    REQUIRE(sess.session_id);
    REQUIRE(sess.resources);
    REQUIRE(sess.priv == nullptr);

    REQUIRE(vaccel_sess_free(&sess) == VACCEL_OK);
    REQUIRE(sess.session_id);
    REQUIRE(sess.hint == 1);
    REQUIRE(sess.resources == nullptr);
    REQUIRE(sess.priv == nullptr);
    
    ret = plugins_shutdown();
    REQUIRE(ret == VACCEL_OK);
    
    ret = sessions_cleanup();
    REQUIRE(ret == VACCEL_OK);


}

// Test case for session update and cleanup
TEST_CASE("vaccel_sess_update_and_free", "[session]")
{

    struct vaccel_session sess;
    sess.session_id = 0;
    sess.resources = nullptr;
    sess.priv = nullptr;
    int ret;

    ret = sessions_bootstrap();
    REQUIRE(ret == VACCEL_OK);

    ret = vaccel_sess_init(&sess, 1);
    REQUIRE(ret == VACCEL_OK);
    REQUIRE(sess.session_id);
    REQUIRE(sess.hint == 1);
    REQUIRE(sess.resources);
    REQUIRE(sess.priv == nullptr);

    // Test session update
    ret = vaccel_sess_update(&sess, 2);
    REQUIRE(ret == VACCEL_OK);
    REQUIRE(sess.session_id);
    REQUIRE(sess.hint == 2);
    REQUIRE(sess.resources);
    REQUIRE(sess.priv == nullptr);

    ret = vaccel_sess_update(NULL, 2);
    REQUIRE(ret == VACCEL_EINVAL);
    REQUIRE(sess.session_id);
    REQUIRE(sess.hint == 2);
    REQUIRE(sess.resources);
    REQUIRE(sess.priv == nullptr);

    ret = vaccel_sess_free(NULL);
    REQUIRE(ret == EINVAL);
    REQUIRE(sess.session_id);
    REQUIRE(sess.hint == 2);
    REQUIRE(sess.resources);
    REQUIRE(sess.priv == nullptr);

    // Test session cleanup
    REQUIRE(vaccel_sess_free(&sess) == VACCEL_OK);
    REQUIRE(sess.session_id);
    REQUIRE(sess.hint == 2);
    REQUIRE(sess.resources == nullptr);
    REQUIRE(sess.priv == nullptr);

    ret = sessions_cleanup();
    REQUIRE(ret == VACCEL_OK);
}

// Test case for unregistering a session with null parameters
TEST_CASE("sess_unregister_null", "[session]")
{
    int ret;

    ret = sessions_bootstrap();
    REQUIRE(VACCEL_OK == ret);

    struct vaccel_session sess;
    sess.session_id = 0;
    sess.resources = nullptr;
    sess.priv = nullptr;

    ret = vaccel_sess_init(&sess, 1);
    REQUIRE(ret == VACCEL_OK);
    REQUIRE(sess.session_id);
    REQUIRE(sess.hint == 1);
    REQUIRE(sess.resources);
    REQUIRE(sess.priv == nullptr);

    struct vaccel_resource res;
    res.type = VACCEL_RES_SHARED_OBJ;

    ret = vaccel_sess_register(NULL, NULL);
    REQUIRE(ret == VACCEL_EINVAL);
    ret = vaccel_sess_register(NULL, &res);
    REQUIRE(ret == VACCEL_EINVAL);
    ret = vaccel_sess_register(&sess, NULL);
    REQUIRE(ret == VACCEL_EINVAL);

    REQUIRE(sess.session_id);
    REQUIRE(sess.hint == 1);
    REQUIRE(sess.resources);
    REQUIRE(sess.priv == nullptr);

    ret = vaccel_sess_register(&sess, &res);
    REQUIRE(ret == VACCEL_OK);
    REQUIRE(sess.session_id);
    REQUIRE(sess.hint == 1);
    REQUIRE_FALSE(list_empty(&sess.resources->registered[res.type]));
    REQUIRE(sess.priv == nullptr);

    bool check_bool = vaccel_sess_has_resource(&sess, &res);
    REQUIRE(check_bool);

    ret = vaccel_sess_unregister(NULL, &res);
    REQUIRE(ret == VACCEL_EINVAL);
    ret = vaccel_sess_unregister(&sess, NULL);
    REQUIRE(ret == VACCEL_EINVAL);

    REQUIRE(sess.session_id);
    REQUIRE(sess.hint == 1);
    REQUIRE_FALSE(list_empty(&sess.resources->registered[res.type]));
    REQUIRE(sess.priv == nullptr);

    res.type = VACCEL_RES_MAX;
    ret = vaccel_sess_unregister(&sess, &res);
    REQUIRE(ret == VACCEL_EINVAL);

    res.type = VACCEL_RES_SHARED_OBJ;
    ret = vaccel_sess_unregister(&sess, &res);
    REQUIRE(ret == VACCEL_OK);
    REQUIRE(sess.session_id);
    REQUIRE(sess.hint == 1);
    REQUIRE(list_empty(&sess.resources->registered[res.type]));
    REQUIRE(sess.priv == nullptr);

    check_bool = vaccel_sess_has_resource(&sess, &res);
    REQUIRE(!check_bool);

    ret = vaccel_sess_free(&sess);
    REQUIRE(ret == VACCEL_OK);

    REQUIRE(sess.session_id);
    REQUIRE(sess.hint == 1);
    REQUIRE(sess.resources == nullptr);
    REQUIRE(sess.priv == nullptr);

    ret = sessions_cleanup();
    REQUIRE(VACCEL_OK == ret);
}

// Test case for session initialization, update, registration, and cleanup
TEST_CASE("session_sess", "[session]")
{
    int ret;
    struct vaccel_session test_sess;
    test_sess.hint = 0;
    test_sess.session_id = 0;
    test_sess.resources = nullptr;
    test_sess.priv = nullptr;
    struct vaccel_resource test_res;
    test_res.type = VACCEL_RES_SHARED_OBJ;

    ret = sessions_bootstrap();
    REQUIRE(VACCEL_OK == ret);

    ret = resources_bootstrap();
    REQUIRE(VACCEL_OK == ret);

    ret = vaccel_sess_init(&test_sess, 1);
    REQUIRE(VACCEL_OK == ret);

    REQUIRE(ret == VACCEL_OK);
    REQUIRE(test_sess.session_id);
    REQUIRE(test_sess.hint == 1);
    REQUIRE(test_sess.resources);
    REQUIRE(test_sess.priv == nullptr);

    ret = vaccel_sess_update(&test_sess, 2);
    REQUIRE(VACCEL_OK == ret);
    REQUIRE(test_sess.session_id);
    REQUIRE(test_sess.hint == 2);
    REQUIRE(test_sess.resources);
    REQUIRE(test_sess.priv == nullptr);

    ret = vaccel_sess_register(&test_sess, &test_res);
    REQUIRE(VACCEL_OK == ret);
    REQUIRE(test_sess.session_id);
    REQUIRE(test_sess.hint == 2);
    REQUIRE_FALSE(list_empty(&test_sess.resources->registered[test_res.type]));
    REQUIRE(test_sess.priv == nullptr);

    ret = vaccel_sess_unregister(&test_sess, &test_res);
    REQUIRE(VACCEL_OK == ret);
    REQUIRE(VACCEL_OK == ret);
    REQUIRE(test_sess.session_id);
    REQUIRE(test_sess.hint == 2);
    REQUIRE(list_empty(&test_sess.resources->registered[test_res.type]));
    REQUIRE(test_sess.priv == nullptr);

    ret = vaccel_sess_free(&test_sess);
    REQUIRE(VACCEL_OK == ret);
    REQUIRE(test_sess.session_id);
    REQUIRE(test_sess.hint == 2);
    REQUIRE(test_sess.resources == nullptr);
    REQUIRE(test_sess.priv == nullptr);

    ret = resources_cleanup();
    REQUIRE(VACCEL_OK == ret);

    ret = sessions_cleanup();
    REQUIRE(VACCEL_OK == ret);
}

// Test case for session initialization, update, registration, and cleanup with
// a VirtIO plugin
TEST_CASE("session_sess_virtio", "[session]")
{
    int ret;
    struct vaccel_session test_sess;
    test_sess.hint = 0;
    test_sess.session_id = 1;
    test_sess.resources = nullptr;
    test_sess.priv = nullptr;
    struct vaccel_resource test_res;
    test_res.type = VACCEL_RES_SHARED_OBJ;

    RESET_FAKE(get_virtio_plugin);

    // Create a mock plugin
    struct vaccel_plugin_info v_mock_info;
    v_mock_info.name = "fake_virtio";
    v_mock_info.sess_init = mock_sess_init;
    v_mock_info.sess_free = mock_sess_free;
    v_mock_info.sess_update = mock_sess_update;
    v_mock_info.sess_register = mock_sess_register;
    v_mock_info.sess_unregister = mock_sess_unregister;

    struct vaccel_plugin v_mock;
    v_mock.info = &v_mock_info;

    get_virtio_plugin_fake.return_val = &v_mock;

    ret = sessions_bootstrap();
    REQUIRE(VACCEL_OK == ret);

    ret = resources_bootstrap();
    REQUIRE(VACCEL_OK == ret);

    ret = vaccel_sess_init(&test_sess, 1);
    REQUIRE(VACCEL_OK == ret);

    ret = vaccel_sess_update(&test_sess, 2);
    REQUIRE(VACCEL_OK == ret);

    ret = vaccel_sess_register(&test_sess, &test_res);
    REQUIRE(VACCEL_OK == ret);

    ret = vaccel_sess_unregister(&test_sess, &test_res);
    REQUIRE(VACCEL_OK == ret);

    ret = vaccel_sess_free(&test_sess);
    REQUIRE(VACCEL_OK == ret);

    // Ensure that the VirtIO plugin was called the expected number of times
    REQUIRE(get_virtio_plugin_fake.call_count == 5);

    // Cleanup session and resource systems
    // ret = resources_cleanup();
    // REQUIRE(VACCEL_OK == ret);

    // ret = sessions_cleanup();
    // REQUIRE(VACCEL_OK == ret);
}
