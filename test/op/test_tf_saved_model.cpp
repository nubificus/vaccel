/*
 * TensorFlow Saved Model from Memory Test
 *
 * The code below tests the creation and registration of a TensorFlow saved
 * model from memory.
 *
 */

#include <catch.hpp>
#include <utils.hpp>

extern "C" {
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <vaccel.h>
}

TEST_CASE("saved_tf_model_from_memory")
{
    int ret;
    const char* path = abs_path(SOURCE_ROOT, "examples/models/tf/lstm2/");

    struct vaccel_tf_saved_model* model = vaccel_tf_saved_model_new();
    REQUIRE(model);

    size_t len;
    unsigned char* ptr = read_file_from_dir(path, "saved_model.pb", &len);
    REQUIRE(ptr);
    REQUIRE(len);

    ret = vaccel_tf_saved_model_set_model(model, ptr, len);
    REQUIRE(ret == VACCEL_OK);

    ptr = read_file_from_dir(path, "variables/variables.index", &len);
    REQUIRE(ptr);
    REQUIRE(len);

    ret = vaccel_tf_saved_model_set_checkpoint(model, ptr, len);
    REQUIRE(ptr);
    REQUIRE(len);

    ptr = read_file_from_dir(path, "variables/variables.index", &len);
    REQUIRE(ptr);
    REQUIRE(len);
    ret = vaccel_tf_saved_model_set_var_index(model, ptr, len);
    REQUIRE(ret == VACCEL_OK);

    ret = vaccel_tf_saved_model_register(model);
    REQUIRE(ret == VACCEL_OK);

    vaccel_id_t model_id = vaccel_tf_saved_model_id(model);
    vaccel_info("Registered new resource: %ld", model_id);

    struct vaccel_session sess;
    ret = vaccel_sess_init(&sess, 0);
    REQUIRE(ret == VACCEL_OK);

    vaccel_info("Registering model %ld with session %u", model_id,
        sess.session_id);

    ret = vaccel_sess_register(&sess, model->resource);
    REQUIRE(ret == VACCEL_OK);

    ret = vaccel_sess_unregister(&sess, model->resource);
    REQUIRE(ret == VACCEL_OK);

    ret = vaccel_tf_saved_model_destroy(model);
    REQUIRE(ret == VACCEL_OK);

    ret = vaccel_sess_free(&sess);
    REQUIRE(ret == VACCEL_OK);
}

TEST_CASE("saved_model_from_file")
{
    int ret;
    const char* path = abs_path(SOURCE_ROOT, "examples/models/tf/lstm2/");

    struct vaccel_tf_saved_model* model = vaccel_tf_saved_model_new();
    REQUIRE(model);

    ret = vaccel_tf_saved_model_set_path(model, path);
    REQUIRE(ret == VACCEL_OK);

    ret = vaccel_tf_saved_model_register(model);
    REQUIRE(ret == VACCEL_OK);

    vaccel_id_t model_id = vaccel_tf_saved_model_id(model);
    vaccel_info("Registered new resource: %ld", model_id);

    struct vaccel_session sess;
    ret = vaccel_sess_init(&sess, 0);
    REQUIRE(ret == VACCEL_OK);

    ret = vaccel_sess_register(&sess, model->resource);
    REQUIRE(ret == VACCEL_OK);

    ret = vaccel_sess_unregister(&sess, model->resource);
    REQUIRE(ret == VACCEL_OK);

    ret = vaccel_tf_saved_model_destroy(model);
    REQUIRE(ret == VACCEL_OK);

    ret = vaccel_sess_free(&sess);
    REQUIRE(ret == VACCEL_OK);
}
