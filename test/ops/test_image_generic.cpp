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
 *
 */

#include "utils.hpp"
#include "vaccel.h"
#include <catch.hpp>
#include <cstdint>
#include <cstdio>
#include <cstdlib>

TEST_CASE("classify_generic", "[ops][image][generic]")
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

	uint32_t image_size_uint32 = 0;
	if (image_size <= UINT32_MAX) {
		image_size_uint32 = static_cast<uint32_t>(image_size);
	} else {
		REQUIRE(1 == 2); // lets fail the test here
	}

	vaccel_op_t op_type = VACCEL_IMG_CLASS;
	struct vaccel_arg read[2] = {
		{ .argtype = 0, .size = sizeof(vaccel_op_t), .buf = &op_type },
		{ .argtype = 0, .size = image_size_uint32, .buf = image }
	};

	struct vaccel_arg write[2] = {
		{ .argtype = 0, .size = sizeof(out_text), .buf = out_text },
		{ .argtype = 0,
		  .size = sizeof(out_imagename),
		  .buf = out_imagename }
	};

	int const ret = vaccel_genop(&sess, read, 2, write, 2);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(out_text);
	REQUIRE(out_imagename);

	printf("classification imagename: %s\n", out_imagename);
	printf("classification tags: %s\n", out_text);

	REQUIRE(vaccel_session_release(&sess) == VACCEL_OK);

	free(file_path);
	free(image);
}

TEST_CASE("depth_generic", "[ops][image][generic]")
{
	char *file_path = abs_path(SOURCE_ROOT, "examples/images/example.jpg");
	int ret;
	char *image;
	size_t image_size = 0;
	char out_imagename[512];
	struct vaccel_session sess;

	REQUIRE(vaccel_session_init(&sess, 0) == VACCEL_OK);

	REQUIRE(fs_file_read(file_path, (void **)&image, &image_size) ==
		VACCEL_OK);

	uint32_t image_size_uint32 = 0;
	if (image_size <= UINT32_MAX) {
		image_size_uint32 = static_cast<uint32_t>(image_size);
	} else {
		REQUIRE(1 == 2); // lets fail the test here
	}

	vaccel_op_t op_type = VACCEL_IMG_DEPTH;
	struct vaccel_arg read[2] = {
		{ .argtype = 0, .size = sizeof(vaccel_op_t), .buf = &op_type },
		{ .argtype = 0, .size = image_size_uint32, .buf = image }
	};

	struct vaccel_arg write[1] = { { .argtype = 0,
					 .size = sizeof(out_imagename),
					 .buf = out_imagename } };

	ret = vaccel_genop(&sess, read, 2, write, 1);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(out_imagename);

	printf("depth estimation imagename: %s\n", out_imagename);

	REQUIRE(vaccel_session_release(&sess) == VACCEL_OK);

	free(file_path);
	free(image);
}

TEST_CASE("detect_generic", "[ops][image][generic]")
{
	char *file_path = abs_path(SOURCE_ROOT, "examples/images/example.jpg");
	int ret;
	char *image;
	size_t image_size = 0;
	char out_imagename[512];
	struct vaccel_session sess;

	REQUIRE(vaccel_session_init(&sess, 0) == VACCEL_OK);

	REQUIRE(fs_file_read(file_path, (void **)&image, &image_size) ==
		VACCEL_OK);

	uint32_t image_size_uint32 = 0;
	if (image_size <= UINT32_MAX) {
		image_size_uint32 = static_cast<uint32_t>(image_size);
	} else {
		REQUIRE(1 == 2); // lets fail the test here
	}

	vaccel_op_t op_type = VACCEL_IMG_DETEC;
	struct vaccel_arg read[2] = {
		{ .argtype = 0, .size = sizeof(vaccel_op_t), .buf = &op_type },
		{ .argtype = 0, .size = image_size_uint32, .buf = image }
	};
	struct vaccel_arg write[1] = {
		{ .argtype = 0,
		  .size = sizeof(out_imagename),
		  .buf = out_imagename },
	};

	ret = vaccel_genop(&sess, read, 2, write, 1);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(out_imagename);

	printf("detection imagename: %s\n", out_imagename);

	REQUIRE(vaccel_session_release(&sess) == VACCEL_OK);

	free(file_path);
	free(image);
}

TEST_CASE("pose_generic", "[ops][image][generic]")
{
	char *file_path = abs_path(SOURCE_ROOT, "examples/images/example.jpg");
	int ret;
	char *image;
	size_t image_size = 0;
	char out_imagename[512];
	struct vaccel_session sess;

	REQUIRE(vaccel_session_init(&sess, 0) == VACCEL_OK);

	REQUIRE(fs_file_read(file_path, (void **)&image, &image_size) ==
		VACCEL_OK);

	uint32_t image_size_uint32 = 0;
	if (image_size <= UINT32_MAX) {
		image_size_uint32 = static_cast<uint32_t>(image_size);
	} else {
		REQUIRE(1 == 2); // lets fail the test here
	}

	vaccel_op_t op_type = VACCEL_IMG_POSE;
	struct vaccel_arg read[2] = {
		{ .argtype = 0, .size = sizeof(vaccel_op_t), .buf = &op_type },
		{ .argtype = 0, .size = image_size_uint32, .buf = image }
	};
	struct vaccel_arg write[1] = {
		{ .argtype = 0,
		  .size = sizeof(out_imagename),
		  .buf = out_imagename },
	};

	ret = vaccel_genop(&sess, read, 2, write, 1);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(out_imagename);

	printf("pose estimation imagename: %s\n", out_imagename);

	REQUIRE(vaccel_session_release(&sess) == VACCEL_OK);

	free(file_path);
	free(image);
}

TEST_CASE("segmentation_generic", "[ops][image][generic]")
{
	char *file_path = abs_path(SOURCE_ROOT, "examples/images/example.jpg");
	int ret;
	char *image;
	size_t image_size = 0;
	char out_imagename[512];
	struct vaccel_session sess;

	REQUIRE(vaccel_session_init(&sess, 0) == VACCEL_OK);

	REQUIRE(fs_file_read(file_path, (void **)&image, &image_size) ==
		VACCEL_OK);

	uint32_t image_size_uint32 = 0;
	if (image_size <= UINT32_MAX) {
		image_size_uint32 = static_cast<uint32_t>(image_size);
	} else {
		REQUIRE(1 == 2); // lets fail the test here
	}

	vaccel_op_t op_type = VACCEL_IMG_SEGME;
	struct vaccel_arg read[2] = {
		{ .argtype = 0, .size = sizeof(vaccel_op_t), .buf = &op_type },
		{ .argtype = 0, .size = image_size_uint32, .buf = image }
	};
	struct vaccel_arg write[1] = {
		{ .argtype = 0,
		  .size = sizeof(out_imagename),
		  .buf = out_imagename },
	};

	ret = vaccel_genop(&sess, read, 2, write, 1);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(out_imagename);

	printf("segmentation output: %s\n", out_imagename);

	REQUIRE(vaccel_session_release(&sess) == VACCEL_OK);

	free(file_path);
	free(image);
}
