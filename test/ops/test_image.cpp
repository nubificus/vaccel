// SPDX-License-Identifier: Apache-2.0

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

#include <cstdio>
#include <cstdlib>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <vaccel.h>

TEST_CASE("classify", "[ops][image]")
{
	char *file_path = abs_path(SOURCE_ROOT, "examples/images/example.jpg");
	char *image;
	size_t image_size = 0;
	char out_text[512];
	char out_imagename[512];
	struct vaccel_session sess;

	REQUIRE(vaccel_session_init(&sess, 0) == VACCEL_OK);

	REQUIRE(fs_file_read(file_path, (void **)&image, &image_size) ==
		VACCEL_OK);

	int ret = vaccel_image_classification(&sess, image,
					      (unsigned char *)out_text,
					      (unsigned char *)out_imagename,
					      image_size, sizeof(out_text),
					      sizeof(out_imagename));
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(out_text);
	REQUIRE(out_imagename);

	printf("classification imagename: %s\n", out_imagename);
	printf("classification tags: %s\n", out_text);

	REQUIRE(vaccel_session_release(&sess) == VACCEL_OK);

	free(file_path);
	free(image);
}

TEST_CASE("depth", "[ops][image]")
{
	char *file_path = abs_path(SOURCE_ROOT, "examples/images/example.jpg");
	char *image;
	size_t image_size = 0;
	char out_imagename[512];
	struct vaccel_session sess;

	REQUIRE(vaccel_session_init(&sess, 0) == VACCEL_OK);

	REQUIRE(fs_file_read(file_path, (void **)&image, &image_size) ==
		VACCEL_OK);

	int ret = vaccel_image_depth(&sess, image,
				     (unsigned char *)out_imagename, image_size,
				     sizeof(out_imagename));
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(out_imagename);

	printf("depth estimation imagename: %s\n", out_imagename);

	REQUIRE(vaccel_session_release(&sess) == VACCEL_OK);

	free(file_path);
	free(image);
}

TEST_CASE("detect", "[ops][image]")
{
	char *file_path = abs_path(SOURCE_ROOT, "examples/images/example.jpg");
	char *image;
	size_t image_size = 0;
	char out_imagename[512];
	struct vaccel_session sess;

	REQUIRE(vaccel_session_init(&sess, 0) == VACCEL_OK);

	REQUIRE(fs_file_read(file_path, (void **)&image, &image_size) ==
		VACCEL_OK);

	int ret = vaccel_image_detection(&sess, image,
					 (unsigned char *)out_imagename,
					 image_size, sizeof(out_imagename));
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(out_imagename);

	printf("detection imagename: %s\n", out_imagename);

	REQUIRE(vaccel_session_release(&sess) == VACCEL_OK);

	free(file_path);
	free(image);
}

TEST_CASE("pose", "[ops][image]")
{
	char *file_path = abs_path(SOURCE_ROOT, "examples/images/example.jpg");
	char *image;
	size_t image_size = 0;
	char out_imagename[512];
	struct vaccel_session sess;

	REQUIRE(vaccel_session_init(&sess, 0) == VACCEL_OK);

	REQUIRE(fs_file_read(file_path, (void **)&image, &image_size) ==
		VACCEL_OK);

	int ret = vaccel_image_pose(&sess, image,
				    (unsigned char *)out_imagename, image_size,
				    sizeof(out_imagename));
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(out_imagename);

	printf("pose estimation imagename: %s\n", out_imagename);

	REQUIRE(vaccel_session_release(&sess) == VACCEL_OK);

	free(file_path);
	free(image);
}

TEST_CASE("segmentation", "[ops][image]")
{
	char *file_path = abs_path(SOURCE_ROOT, "examples/images/example.jpg");
	char *image;
	size_t image_size = 0;
	char out_imagename[512];
	struct vaccel_session sess;

	REQUIRE(vaccel_session_init(&sess, 0) == VACCEL_OK);

	REQUIRE(fs_file_read(file_path, (void **)&image, &image_size) ==
		VACCEL_OK);

	int ret = vaccel_image_segmentation(&sess, image,
					    (unsigned char *)out_imagename,
					    image_size, sizeof(out_imagename));
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(out_imagename);

	printf("segmentation output: %s\n", out_imagename);

	REQUIRE(vaccel_session_release(&sess) == VACCEL_OK);

	free(file_path);
	free(image);
}
