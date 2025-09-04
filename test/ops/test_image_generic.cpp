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
	unsigned char out_text[512];
	unsigned char out_imagename[512];
	struct vaccel_session sess;

	REQUIRE(vaccel_session_init(&sess, 0) == VACCEL_OK);

	REQUIRE(fs_file_read(file_path, (void **)&image, &image_size) ==
		VACCEL_OK);

	struct vaccel_arg_array read_args;
	struct vaccel_arg_array write_args;
	REQUIRE(vaccel_arg_array_init(&read_args, 2) == VACCEL_OK);
	REQUIRE(vaccel_arg_array_init(&write_args, 2) == VACCEL_OK);

	const auto op_type = (uint8_t)VACCEL_OP_IMAGE_CLASSIFY;
	REQUIRE(vaccel_arg_array_add_uint8(&read_args, (uint8_t *)&op_type) ==
		VACCEL_OK);
	REQUIRE(vaccel_arg_array_add_buffer(&read_args, image, image_size) ==
		VACCEL_OK);
	REQUIRE(vaccel_arg_array_add_uchar_array(
			&write_args, out_imagename,
			sizeof(out_imagename) / sizeof(out_imagename[0])) ==
		VACCEL_OK);
	REQUIRE(vaccel_arg_array_add_uchar_array(
			&write_args, out_text,
			sizeof(out_text) / sizeof(out_text[0])) == VACCEL_OK);

	int const ret = vaccel_genop(&sess, read_args.args, read_args.count,
				     write_args.args, write_args.count);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(out_text);
	REQUIRE(out_imagename);

	printf("classification imagename: %s\n", out_imagename);
	printf("classification tags: %s\n", out_text);

	REQUIRE(vaccel_session_release(&sess) == VACCEL_OK);
	REQUIRE(vaccel_arg_array_release(&read_args) == VACCEL_OK);
	REQUIRE(vaccel_arg_array_release(&write_args) == VACCEL_OK);
	free(file_path);
	free(image);
}

TEST_CASE("depth_generic", "[ops][image][generic]")
{
	char *file_path = abs_path(SOURCE_ROOT, "examples/images/example.jpg");
	char *image;
	size_t image_size = 0;
	unsigned char out_imagename[512];
	struct vaccel_session sess;

	REQUIRE(vaccel_session_init(&sess, 0) == VACCEL_OK);

	REQUIRE(fs_file_read(file_path, (void **)&image, &image_size) ==
		VACCEL_OK);

	struct vaccel_arg_array read_args;
	struct vaccel_arg_array write_args;
	REQUIRE(vaccel_arg_array_init(&read_args, 2) == VACCEL_OK);
	REQUIRE(vaccel_arg_array_init(&write_args, 1) == VACCEL_OK);

	const auto op_type = (uint8_t)VACCEL_OP_IMAGE_DEPTH;
	REQUIRE(vaccel_arg_array_add_uint8(&read_args, (uint8_t *)&op_type) ==
		VACCEL_OK);
	REQUIRE(vaccel_arg_array_add_buffer(&read_args, image, image_size) ==
		VACCEL_OK);
	REQUIRE(vaccel_arg_array_add_uchar_array(
			&write_args, out_imagename,
			sizeof(out_imagename) / sizeof(out_imagename[0])) ==
		VACCEL_OK);

	int const ret = vaccel_genop(&sess, read_args.args, read_args.count,
				     write_args.args, write_args.count);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(out_imagename);

	REQUIRE(vaccel_session_release(&sess) == VACCEL_OK);
	REQUIRE(vaccel_arg_array_release(&read_args) == VACCEL_OK);
	REQUIRE(vaccel_arg_array_release(&write_args) == VACCEL_OK);
	free(file_path);
	free(image);
}

TEST_CASE("detect_generic", "[ops][image][generic]")
{
	char *file_path = abs_path(SOURCE_ROOT, "examples/images/example.jpg");
	char *image;
	size_t image_size = 0;
	unsigned char out_imagename[512];
	struct vaccel_session sess;

	REQUIRE(vaccel_session_init(&sess, 0) == VACCEL_OK);

	REQUIRE(fs_file_read(file_path, (void **)&image, &image_size) ==
		VACCEL_OK);

	struct vaccel_arg_array read_args;
	struct vaccel_arg_array write_args;
	REQUIRE(vaccel_arg_array_init(&read_args, 2) == VACCEL_OK);
	REQUIRE(vaccel_arg_array_init(&write_args, 1) == VACCEL_OK);

	const auto op_type = (uint8_t)VACCEL_OP_IMAGE_DETECT;
	REQUIRE(vaccel_arg_array_add_uint8(&read_args, (uint8_t *)&op_type) ==
		VACCEL_OK);
	REQUIRE(vaccel_arg_array_add_buffer(&read_args, image, image_size) ==
		VACCEL_OK);
	REQUIRE(vaccel_arg_array_add_uchar_array(
			&write_args, out_imagename,
			sizeof(out_imagename) / sizeof(out_imagename[0])) ==
		VACCEL_OK);

	int const ret = vaccel_genop(&sess, read_args.args, read_args.count,
				     write_args.args, write_args.count);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(out_imagename);

	REQUIRE(vaccel_session_release(&sess) == VACCEL_OK);
	REQUIRE(vaccel_arg_array_release(&read_args) == VACCEL_OK);
	REQUIRE(vaccel_arg_array_release(&write_args) == VACCEL_OK);
	free(file_path);
	free(image);
}

TEST_CASE("pose_generic", "[ops][image][generic]")
{
	char *file_path = abs_path(SOURCE_ROOT, "examples/images/example.jpg");
	char *image;
	size_t image_size = 0;
	unsigned char out_imagename[512];
	struct vaccel_session sess;

	REQUIRE(vaccel_session_init(&sess, 0) == VACCEL_OK);

	REQUIRE(fs_file_read(file_path, (void **)&image, &image_size) ==
		VACCEL_OK);

	struct vaccel_arg_array read_args;
	struct vaccel_arg_array write_args;
	REQUIRE(vaccel_arg_array_init(&read_args, 2) == VACCEL_OK);
	REQUIRE(vaccel_arg_array_init(&write_args, 1) == VACCEL_OK);

	const auto op_type = (uint8_t)VACCEL_OP_IMAGE_POSE;
	REQUIRE(vaccel_arg_array_add_uint8(&read_args, (uint8_t *)&op_type) ==
		VACCEL_OK);
	REQUIRE(vaccel_arg_array_add_buffer(&read_args, image, image_size) ==
		VACCEL_OK);
	REQUIRE(vaccel_arg_array_add_uchar_array(
			&write_args, out_imagename,
			sizeof(out_imagename) / sizeof(out_imagename[0])) ==
		VACCEL_OK);

	int const ret = vaccel_genop(&sess, read_args.args, read_args.count,
				     write_args.args, write_args.count);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(out_imagename);

	REQUIRE(vaccel_session_release(&sess) == VACCEL_OK);
	REQUIRE(vaccel_arg_array_release(&read_args) == VACCEL_OK);
	REQUIRE(vaccel_arg_array_release(&write_args) == VACCEL_OK);
	free(file_path);
	free(image);
}

TEST_CASE("segmentation_generic", "[ops][image][generic]")
{
	char *file_path = abs_path(SOURCE_ROOT, "examples/images/example.jpg");
	char *image;
	size_t image_size = 0;
	unsigned char out_imagename[512];
	struct vaccel_session sess;

	REQUIRE(vaccel_session_init(&sess, 0) == VACCEL_OK);

	REQUIRE(fs_file_read(file_path, (void **)&image, &image_size) ==
		VACCEL_OK);

	struct vaccel_arg_array read_args;
	struct vaccel_arg_array write_args;
	REQUIRE(vaccel_arg_array_init(&read_args, 2) == VACCEL_OK);
	REQUIRE(vaccel_arg_array_init(&write_args, 1) == VACCEL_OK);

	const auto op_type = (uint8_t)VACCEL_OP_IMAGE_SEGMENT;
	REQUIRE(vaccel_arg_array_add_uint8(&read_args, (uint8_t *)&op_type) ==
		VACCEL_OK);
	REQUIRE(vaccel_arg_array_add_buffer(&read_args, image, image_size) ==
		VACCEL_OK);
	REQUIRE(vaccel_arg_array_add_uchar_array(
			&write_args, out_imagename,
			sizeof(out_imagename) / sizeof(out_imagename[0])) ==
		VACCEL_OK);

	int const ret = vaccel_genop(&sess, read_args.args, read_args.count,
				     write_args.args, write_args.count);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(out_imagename);

	REQUIRE(vaccel_session_release(&sess) == VACCEL_OK);
	REQUIRE(vaccel_arg_array_release(&read_args) == VACCEL_OK);
	REQUIRE(vaccel_arg_array_release(&write_args) == VACCEL_OK);
	free(file_path);
	free(image);
}
