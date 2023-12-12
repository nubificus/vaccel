/*
 * Unit Testing for exec
 *
 * The code below performs unit testing for VAccel library executions.
 * It includes test cases for the `exec`, `exec_generic`, and
 * `exec_with_resources` functions.
 *
 */

#include <catch.hpp>

#include <atomic>

using atomic_int = std::atomic<int>;
using atomic_uint = std::atomic<unsigned int>;

extern "C" {
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "plugin.h"
#include "session.h"
#include "shared_object.h"
#include <vaccel.h>
}

TEST_CASE("due to vaccel_arg change - TODO: redo these tests")
{
    REQUIRE(1 == 1);
}

TEST_CASE("exec")
{
    int ret;
    int input = 10;
    char out_text[512];
    struct vaccel_session sess;
    sess.session_id = 0;
    sess.priv = nullptr;
    sess.resources = nullptr;
    sess.hint = 0;

    ret = vaccel_sess_init(&sess, VACCEL_PLUGIN_DEBUG);
    REQUIRE(ret == VACCEL_OK);

    struct vaccel_arg read[1] = { { .argtype = 0, .size = sizeof(input), .buf = &input } };
    struct vaccel_arg write[1] = { { .argtype = 0, .size = sizeof(out_text), .buf = out_text } };

    ret = vaccel_exec(&sess, "../plugins/noop/libvaccel-noop.so", "mytestfunc", read, 1, write, 1);
    REQUIRE(ret == VACCEL_OK);
    REQUIRE(sess.session_id);
    REQUIRE(sess.hint == VACCEL_PLUGIN_DEBUG);
    REQUIRE(sess.resources);
    REQUIRE(sess.priv == nullptr);
}

TEST_CASE("exec_generic")
{
    int ret;

    struct vaccel_session sess;
    sess.session_id = 0;
    sess.priv = nullptr;
    sess.resources = nullptr;
    sess.hint = 0;

    int input;
    char out_text[512];

    ret = vaccel_sess_init(&sess, 0);
    REQUIRE(ret == VACCEL_OK);

    input = 10;
    enum vaccel_op_type op_type = VACCEL_EXEC;

    const char plugin_path[] = "../plugins/noop/libvaccel-noop.so";
    const char function_name[] = "mytestfunc";

    struct vaccel_arg read[4] = {
        { .argtype = 0, .size = sizeof(uint8_t), .buf = &op_type },
        { .argtype = 0, .size = sizeof(plugin_path), .buf = (void*)plugin_path },
        { .argtype = 0, .size = sizeof(function_name), .buf = (void*)function_name },
        { .argtype = 0, .size = sizeof(input), .buf = &input }
    };

    struct vaccel_arg write[1] = {
        { .argtype = 0, .size = sizeof(out_text), .buf = out_text },
    };

    ret = vaccel_genop(&sess, read, 4, write, 1);
    REQUIRE(ret == VACCEL_OK);
    printf("output: %s\n", out_text);
    REQUIRE(sess.session_id);
    REQUIRE(sess.hint == 0);
    REQUIRE(sess.resources);
    REQUIRE(sess.priv == nullptr);
}

extern "C" {
static unsigned char* read_file(const char* path, size_t* len)
{
    struct stat buffer;
    int status, fd;

    fd = open(path, O_RDONLY);
    if (fd < 0) {
        perror("Could not open file");
        return NULL;
    }

    status = fstat(fd, &buffer);
    if (status < 0) {
        perror("Could not stat file");
        return NULL;
    }

    unsigned char* buff = static_cast<unsigned char*>(malloc(buffer.st_size));
    if (!buff) {
        close(fd);
        perror("malloc");
        return NULL;
    }

    size_t bytes = buffer.st_size;
    ssize_t ptr = 0;
    while (bytes) {
        ssize_t ret = read(fd, &buff[ptr], bytes);
        if (ret < 0) {
            perror("read");
            free(buff);
            close(fd);
            return NULL;
        }

        ptr += ret;
        bytes -= ret;
    }

    close(fd);

    *len = ptr;
    return buff;
}
}

TEST_CASE("exec_with_resources")
{

    int input, ret;
    char out_text[512];
    char out_text2[512];
    const char plugin_path[] = "../plugins/noop/libvaccel-noop.so";

    size_t len = 0;
    unsigned char* buff = read_file(plugin_path, &len);
    REQUIRE(buff);

    struct vaccel_shared_object object;
    object.resource = nullptr;
    object.plugin_data = nullptr;

    struct vaccel_shared_object object2;
    object2.resource = nullptr;
    object2.plugin_data = nullptr;

    ret = vaccel_shared_object_new(&object, plugin_path);
    REQUIRE(ret == VACCEL_OK);
    REQUIRE(object.resource);

    ret = vaccel_shared_object_new_from_buffer(&object2, buff, len);
    REQUIRE(ret == VACCEL_OK);
    REQUIRE(object2.resource);

    struct vaccel_session sess;
    sess.session_id = 0;
    sess.priv = nullptr;
    sess.resources = nullptr;
    sess.hint = 1;

    struct vaccel_session sess2;
    sess2.session_id = 0;
    sess2.priv = nullptr;
    sess2.resources = nullptr;
    sess2.hint = 1;

    ret = vaccel_sess_init(&sess, 0);
    REQUIRE(ret == VACCEL_OK);
    REQUIRE(sess.session_id);
    REQUIRE(sess.hint == 0);
    REQUIRE(sess.resources);
    REQUIRE(sess.priv == nullptr);

    ret = vaccel_sess_init(&sess2, 0);
    REQUIRE(ret == VACCEL_OK);
    REQUIRE(sess2.session_id);
    REQUIRE(sess2.hint == 0);
    REQUIRE(sess2.resources);
    REQUIRE(sess2.priv == nullptr);

    ret = vaccel_sess_register(&sess, object.resource);
    REQUIRE(ret == VACCEL_OK);
    REQUIRE(sess.session_id);
    REQUIRE(sess.hint == 0);
    REQUIRE_FALSE(list_empty(&sess.resources->registered[object.resource->type]));
    REQUIRE(sess.priv == nullptr);

    ret = vaccel_sess_register(&sess2, object2.resource);
    REQUIRE(ret == VACCEL_OK);
    REQUIRE(sess2.session_id);
    REQUIRE(sess2.hint == 0);
    REQUIRE_FALSE(list_empty(&sess2.resources->registered[object2.resource->type]));
    REQUIRE(sess2.priv == nullptr);

    input = 10;
    struct vaccel_arg read[1] = { { .argtype = 0, .size = sizeof(input), .buf = &input } };
    struct vaccel_arg write[1] = {
        { .argtype = 0, .size = sizeof(out_text), .buf = out_text },
    };

    struct vaccel_arg read_2[1] = { { .argtype = 0, .size = sizeof(input), .buf = &input } };
    struct vaccel_arg write_2[1] = {
        { .argtype = 0, .size = sizeof(out_text2), .buf = out_text2 },
    };

    ret = vaccel_exec_with_resource(&sess, &object, "mytestfunc", read, 1, write, 1);
    REQUIRE(ret == VACCEL_OK);
    REQUIRE(sess.session_id);
    REQUIRE(sess.hint == 0);
    REQUIRE_FALSE(list_empty(&sess.resources->registered[object.resource->type]));
    REQUIRE(sess.priv == nullptr);

    ret = vaccel_exec_with_resource(&sess2, &object2, "mytestfunc", read_2, 1, write_2, 1);
    REQUIRE(ret == VACCEL_OK);
    REQUIRE(sess2.session_id);
    REQUIRE(sess2.hint == 0);
    REQUIRE_FALSE(list_empty(&sess2.resources->registered[object2.resource->type]));
    REQUIRE(sess2.priv == nullptr);

    ret = vaccel_sess_unregister(&sess, object.resource);
    REQUIRE(ret == VACCEL_OK);
    REQUIRE(sess.session_id);
    REQUIRE(sess.hint == 0);
    REQUIRE(list_empty(&sess.resources->registered[object.resource->type]));
    REQUIRE(sess.priv == nullptr);

    ret = vaccel_sess_unregister(&sess2, object2.resource);
    REQUIRE(ret == VACCEL_OK);
    REQUIRE(sess2.session_id);
    REQUIRE(sess2.hint == 0);
    REQUIRE(list_empty(&sess2.resources->registered[object2.resource->type]));
    REQUIRE(sess2.priv == nullptr);

    ret = vaccel_shared_object_destroy(&object);
    REQUIRE(ret == VACCEL_OK);

    ret = vaccel_shared_object_destroy(&object2);
    REQUIRE(ret == VACCEL_OK);

    ret = vaccel_sess_free(&sess);
    REQUIRE(ret == VACCEL_OK);

    ret = vaccel_sess_free(&sess2);
    REQUIRE(ret == VACCEL_OK);

    free(buff);
}
