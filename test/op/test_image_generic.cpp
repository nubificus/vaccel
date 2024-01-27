/*
 * Unit Testing for VAccel Image operations
 *
 * The code below performs unit testing for various VAccel image processing
 * operations. It includes test cases for image classification, depth
 * estimation, object detection, pose estimation, image segmentation, and their
 * generic counterparts.
 *
 * Each test case initializes a VAccel session, reads an image file, and
 * performs the specified image processing operation in a loop. The results are
 * printed to the console. The test cases also handle memory allocation, error
 * checking, and session cleanup.
 */

#include <catch.hpp>
#include <utils.hpp>

extern "C" {
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <vaccel.h>
}

TEST_CASE("classify_generic")
{
    char* file_path = abs_path(SOURCE_ROOT, "examples/images/example.jpg");
    int ret;
    char* image;
    size_t image_size = 0;
    char out_text[512], out_imagename[512];

    struct vaccel_session sess;
    sess.session_id = 0;
    sess.priv = nullptr;
    sess.resources = nullptr;
    sess.hint = 1;

    ret = vaccel_sess_init(&sess, 0);
    REQUIRE(ret == VACCEL_OK);
    REQUIRE(sess.session_id);
    REQUIRE(sess.hint == 0);
    REQUIRE(sess.resources);
    REQUIRE(sess.priv == nullptr);

    ret = read_file(file_path, &image, &image_size);
    REQUIRE(ret == VACCEL_OK);
    REQUIRE(image_size);
    REQUIRE(&image);

    uint32_t image_size_uint32 = 0;
    if (image_size <= UINT32_MAX) {
        image_size_uint32 = static_cast<uint32_t>(image_size);
    } else {
        REQUIRE(1 == 2); // lets fail the test here
    }

    enum vaccel_op_type op_type = VACCEL_IMG_CLASS;
    struct vaccel_arg read[2] = {
        { .argtype = 0, .size = sizeof(enum vaccel_op_type), .buf = &op_type },
        { .argtype = 0, .size = image_size_uint32, .buf = image }
    };

    struct vaccel_arg write[2] = {
        { .argtype = 0, .size = sizeof(out_text), .buf = out_text },
        { .argtype = 0, .size = sizeof(out_imagename), .buf = out_imagename }
    };

    ret = vaccel_genop(&sess, read, 2, write, 2);

    printf("classification tags: %s\n", out_text);

    ret = vaccel_sess_free(&sess);
    REQUIRE(ret == VACCEL_OK);
}

TEST_CASE("depth_generic")
{
    char* file_path = abs_path(SOURCE_ROOT, "examples/images/example.jpg");
    int ret;
    char* image;
    size_t image_size = 0;
    char out_imagename[512];

    struct vaccel_session sess;
    sess.session_id = 0;
    sess.priv = nullptr;
    sess.resources = nullptr;
    sess.hint = 1;

    ret = vaccel_sess_init(&sess, 0);
    REQUIRE(ret == VACCEL_OK);
    REQUIRE(sess.session_id);
    REQUIRE(sess.hint == 0);
    REQUIRE(sess.resources);
    REQUIRE(sess.priv == nullptr);

    ret = read_file(file_path, &image, &image_size);
    REQUIRE(ret == VACCEL_OK);
    REQUIRE(image_size);
    REQUIRE(&image);

    uint32_t image_size_uint32 = 0;
    if (image_size <= UINT32_MAX) {
        image_size_uint32 = static_cast<uint32_t>(image_size);
    } else {
        REQUIRE(1 == 2); // lets fail the test here
    }

    enum vaccel_op_type op_type = VACCEL_IMG_DEPTH;
    struct vaccel_arg read[2] = {
        { .argtype = 0, .size = sizeof(enum vaccel_op_type), .buf = &op_type },
        { .argtype = 0, .size = image_size_uint32, .buf = image }
    };

    struct vaccel_arg write[1] = {
        { .argtype = 0, .size = sizeof(out_imagename), .buf = out_imagename }
    };

    ret = vaccel_genop(&sess, read, 2, write, 1);
    REQUIRE(ret == VACCEL_OK);
    printf("depth estimation imagename: %s\n", out_imagename);

    ret = vaccel_sess_free(&sess);
    REQUIRE(ret == VACCEL_OK);
}

TEST_CASE("detect_generic")
{
    char* file_path = abs_path(SOURCE_ROOT, "examples/images/example.jpg");
    int ret;
    char* image;
    size_t image_size = 0;
    char out_imagename[512];

    struct vaccel_session sess;
    sess.session_id = 0;
    sess.priv = nullptr;
    sess.resources = nullptr;
    sess.hint = 1;

    ret = vaccel_sess_init(&sess, 0);
    REQUIRE(ret == VACCEL_OK);
    REQUIRE(sess.session_id);
    REQUIRE(sess.hint == 0);
    REQUIRE(sess.resources);
    REQUIRE(sess.priv == nullptr);

    ret = read_file(file_path, &image, &image_size);
    REQUIRE(ret == VACCEL_OK);
    REQUIRE(image_size);
    REQUIRE(&image);

    uint32_t image_size_uint32 = 0;
    if (image_size <= UINT32_MAX) {
        image_size_uint32 = static_cast<uint32_t>(image_size);
    } else {
        REQUIRE(1 == 2); // lets fail the test here
    }

    enum vaccel_op_type op_type = VACCEL_IMG_DETEC;
    struct vaccel_arg read[2] = {
        { .argtype = 0, .size = sizeof(enum vaccel_op_type), .buf = &op_type },
        { .argtype = 0, .size = image_size_uint32, .buf = image }
    };
    struct vaccel_arg write[1] = {
        { .argtype = 0, .size = sizeof(out_imagename), .buf = out_imagename },
    };

    ret = vaccel_genop(&sess, read, 2, write, 1);
    REQUIRE(ret == VACCEL_OK);

    printf("detection image name: %s\n", out_imagename);

    ret = vaccel_sess_free(&sess);
    REQUIRE(ret == VACCEL_OK);
}

TEST_CASE("pose_generic")
{
    char* file_path = abs_path(SOURCE_ROOT, "examples/images/example.jpg");
    int ret;
    char* image;
    size_t image_size = 0;
    char out_imagename[512];

    struct vaccel_session sess;
    sess.session_id = 0;
    sess.priv = nullptr;
    sess.resources = nullptr;
    sess.hint = 1;

    ret = vaccel_sess_init(&sess, 0);
    REQUIRE(ret == VACCEL_OK);
    REQUIRE(sess.session_id);
    REQUIRE(sess.hint == 0);
    REQUIRE(sess.resources);
    REQUIRE(sess.priv == nullptr);

    ret = read_file(file_path, &image, &image_size);
    REQUIRE(ret == VACCEL_OK);
    REQUIRE(image_size);
    REQUIRE(&image);

    uint32_t image_size_uint32 = 0;
    if (image_size <= UINT32_MAX) {
        image_size_uint32 = static_cast<uint32_t>(image_size);
    } else {
        REQUIRE(1 == 2); // lets fail the test here
    }

    enum vaccel_op_type op_type = VACCEL_IMG_POSE;
    struct vaccel_arg read[2] = {
        { .argtype = 0, .size = sizeof(enum vaccel_op_type), .buf = &op_type },
        { .argtype = 0, .size = image_size_uint32, .buf = image }
    };
    struct vaccel_arg write[1] = {
        { .argtype = 0, .size = sizeof(out_imagename), .buf = out_imagename },
    };

    ret = vaccel_genop(&sess, read, 2, write, 1);
    REQUIRE(ret == VACCEL_OK);
    printf("pose estimation imagename: %s\n", out_imagename);

    ret = vaccel_sess_free(&sess);
    REQUIRE(ret == VACCEL_OK);
}

TEST_CASE("segmentation_generic")
{
    char* file_path = abs_path(SOURCE_ROOT, "examples/images/example.jpg");
    int ret;
    char* image;
    size_t image_size = 0;
    char out_imagename[512];

    struct vaccel_session sess;
    sess.session_id = 0;
    sess.priv = nullptr;
    sess.resources = nullptr;
    sess.hint = 1;

    ret = vaccel_sess_init(&sess, 0);
    REQUIRE(ret == VACCEL_OK);
    REQUIRE(sess.session_id);
    REQUIRE(sess.hint == 0);
    REQUIRE(sess.resources);
    REQUIRE(sess.priv == nullptr);

    ret = read_file(file_path, &image, &image_size);
    REQUIRE(ret == VACCEL_OK);
    REQUIRE(image_size);
    REQUIRE(&image);

    uint32_t image_size_uint32 = 0;
    if (image_size <= UINT32_MAX) {
        image_size_uint32 = static_cast<uint32_t>(image_size);
    } else {
        REQUIRE(1 == 2); // lets fail the test here
    }

    enum vaccel_op_type op_type = VACCEL_IMG_SEGME;
    struct vaccel_arg read[2] = {
        { .argtype = 0, .size = sizeof(enum vaccel_op_type), .buf = &op_type },
        { .argtype = 0, .size = image_size_uint32, .buf = image }
    };
    struct vaccel_arg write[1] = {
        { .argtype = 0, .size = sizeof(out_imagename), .buf = out_imagename },
    };

    ret = vaccel_genop(&sess, read, 2, write, 1);
    REQUIRE(ret == VACCEL_OK);
    printf("segmentation output: %s\n", out_imagename);

    ret = vaccel_sess_free(&sess);
    REQUIRE(ret == VACCEL_OK);
}
