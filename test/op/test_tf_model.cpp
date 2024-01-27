/*
 * TensorFlow Model Test
 *
 * The code below tests TensorFlow model handling using vAccel.
 *
 */

#include <catch.hpp>
#include <cstddef>
#include <utils.hpp>

extern "C" {
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <vaccel.h>
}

TEST_CASE("tf_model")
{
    struct vaccel_tf_model model;

    model.resource = nullptr;
    model.plugin_data = nullptr;

    struct vaccel_session sess;
    sess.session_id = 1;
    sess.resources = nullptr;
    sess.priv = nullptr;

    const char* model_path =
	    abs_path(SOURCE_ROOT, "examples/models/tf/lstm2/saved_model.pb");

    int ret = vaccel_tf_model_new(&model, model_path);
    REQUIRE(ret == VACCEL_OK);
    REQUIRE(model.resource);
    REQUIRE(model.plugin_data == nullptr);

    ret = vaccel_sess_init(&sess, 0);
    REQUIRE(ret == VACCEL_OK);
    REQUIRE(sess.hint == 0);
    REQUIRE(sess.session_id == 1);
    REQUIRE(sess.resources);
    REQUIRE(sess.priv == nullptr);
    
    ret = vaccel_sess_register(&sess, model.resource);
    REQUIRE(ret == VACCEL_OK);
    REQUIRE(sess.hint == 0);
    REQUIRE(sess.resources);
    REQUIRE(sess.session_id == 1);
    REQUIRE(sess.priv == nullptr);

    struct vaccel_tf_model model2;
    model2.resource = nullptr;
    model2.plugin_data = nullptr;

    size_t len = 0;
    char* buff;
    ret = read_file(model_path, &buff, &len);
    REQUIRE(ret == VACCEL_OK);
    REQUIRE(buff);
    REQUIRE(len);

    ret = vaccel_tf_model_new_from_buffer(&model2, (unsigned char *)buff, len);
    REQUIRE(ret == VACCEL_OK);
    REQUIRE(model2.resource);
    REQUIRE(model2.plugin_data == nullptr);

    ret = vaccel_sess_register(&sess, model2.resource);
    REQUIRE(ret == VACCEL_OK);

    ret = vaccel_sess_unregister(&sess, model.resource);
    REQUIRE(ret == VACCEL_OK);

    ret = vaccel_sess_unregister(&sess, model2.resource);
    REQUIRE(ret == VACCEL_OK);

    ret = vaccel_tf_model_destroy(&model);
    REQUIRE(ret == VACCEL_OK);
    // REQUIRE(model.resource == nullptr);
    REQUIRE(model.plugin_data == nullptr);

    ret = vaccel_tf_model_destroy(&model2);
    REQUIRE(ret == VACCEL_OK);
    // REQUIRE(model2.resource == nullptr);
    REQUIRE(model2.plugin_data == nullptr);

    ret = vaccel_sess_free(&sess);
    REQUIRE(ret == VACCEL_OK);
    REQUIRE(sess.session_id == 1);
    REQUIRE(sess.hint == 0);
    REQUIRE(sess.resources == nullptr);
    REQUIRE(sess.priv == nullptr);

    free(buff);
}
