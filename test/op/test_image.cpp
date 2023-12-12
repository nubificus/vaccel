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

#include "image.h"
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

#include "session.h"
#include <vaccel.h>
}

extern "C" {

int read_file(const char* filename, char** img, size_t* img_size)
{
    int fd;
    long bytes = 0;

    fd = open(filename, O_RDONLY);
    if (fd < 0) {
        perror("open: ");
        return 1;
    }

    struct stat info;
    fstat(fd, &info);
    fprintf(stdout, "Image size: %luB\n", info.st_size);

    char* buf = (char*)malloc(info.st_size);
    if (!buf) {
        fprintf(stderr, "Could not allocate memory for image\n");
        goto free_buff;
    }

    do {
        int ret = read(fd, buf, info.st_size);
        if (ret < 0) {
            perror("Error while reading image: ");
            goto close_file;
        }
        bytes += ret;
    } while (bytes < info.st_size);

    if (bytes < info.st_size) {
        fprintf(stderr, "Could not read image\n");
        goto close_file;
    }

    *img = buf;
    *img_size = info.st_size;
    close(fd);

    return 0;

close_file:
    close(fd);
free_buff:
    free(buf);
    return 1;
}
}
TEST_CASE("classify")
{
    char file_path[] = "../../examples/images/example.jpg";
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

    ret = vaccel_image_classification(
        &sess, image, (unsigned char*)out_text, (unsigned char*)out_imagename,
        image_size, sizeof(out_text), sizeof(out_imagename));

    printf("classification tags: %s\n", out_text);

    ret = vaccel_sess_free(&sess);
    REQUIRE(ret == VACCEL_OK);
}

TEST_CASE("depth")
{

    char file_path[] = "../../examples/images/example.jpg";
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

    ret = vaccel_image_depth(&sess, image, (unsigned char*)out_imagename,
        image_size, sizeof(out_imagename));
    REQUIRE(ret == VACCEL_OK);
    printf("depth estimation imagename: %s\n", out_imagename);

    ret = vaccel_sess_free(&sess);
    REQUIRE(ret == VACCEL_OK);
}
TEST_CASE("detect")
{

    char file_path[] = "../../examples/images/example.jpg";
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

    ret = vaccel_image_detection(&sess, image, (unsigned char*)out_imagename,
        image_size, sizeof(out_imagename));
    REQUIRE(ret == VACCEL_OK);
    printf("detection image name: %s\n", out_imagename);

    ret = vaccel_sess_free(&sess);
    REQUIRE(ret == VACCEL_OK);
}

TEST_CASE("pose")
{

    char file_path[] = "../../examples/images/example.jpg";
    int ret;
    char* image;
    size_t image_size = 0;
    char out_imagename[512];

    struct vaccel_session sess;
    sess.session_id = 1;
    sess.priv = nullptr;
    sess.resources = nullptr;
    sess.hint = 1;

    ret = vaccel_sess_init(&sess, 0);
    REQUIRE(ret == VACCEL_OK);
    REQUIRE(sess.session_id == 1);
    REQUIRE(sess.hint == 0);
    REQUIRE(sess.resources);
    REQUIRE(sess.priv == nullptr);

    ret = read_file(file_path, &image, &image_size);
    REQUIRE(ret == VACCEL_OK);
    REQUIRE(image_size);
    REQUIRE(&image);

    ret = vaccel_image_pose(&sess, image, (unsigned char*)out_imagename,
        image_size, sizeof(out_imagename));
    REQUIRE(ret == VACCEL_OK);
    printf("pose estimation imagename: %s\n", out_imagename);

    ret = vaccel_sess_free(&sess);
    REQUIRE(ret == VACCEL_OK);
    REQUIRE(sess.session_id == 1);
    REQUIRE(sess.hint == 0);
    REQUIRE(sess.resources == nullptr);
    REQUIRE(sess.priv == nullptr);
}

TEST_CASE("segmentation")
{

    char file_path[] = "../../examples/images/example.jpg";
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

    ret = vaccel_image_segmentation(&sess, image, (unsigned char*)out_imagename,
        image_size, sizeof(out_imagename));
    REQUIRE(ret == VACCEL_OK);
    printf("segmentation output: %s\n", out_imagename);

    ret = vaccel_sess_free(&sess);
    REQUIRE(ret == VACCEL_OK);
}
