// SPDX-License-Identifier: Apache-2.0

/*
 * Unit Testing for exec
 *
 * The code below performs unit testing for VAccel library executions.
 * It includes test cases for the `exec`, `exec_generic`, and
 * `exec_with_resources` functions.
 *
 */

#include "utils.hpp"
#include "vaccel.h"
#include <catch.hpp>
#include <cstdint>
#include <cstdio>
#include <cstdlib>

// TODO: Add arg_helpers tests

TEST_CASE("exec", "[ops][exec]")
{
	int ret;
	int input = 10;
	int output = 0;
	struct vaccel_session sess;

	REQUIRE(vaccel_session_init(&sess, 0) == VACCEL_OK);

	struct vaccel_arg read[1] = {
		{ .argtype = 0, .size = sizeof(input), .buf = &input }
	};
	struct vaccel_arg write[1] = {
		{ .argtype = 0, .size = sizeof(output), .buf = &output }
	};

	char *lib_path = abs_path(BUILD_ROOT, "examples/libmytestlib.so");
	const char function_name[] = "mytestfunc";

	ret = vaccel_exec(&sess, lib_path, function_name, read, 1, write, 1);
	REQUIRE(ret == VACCEL_OK);
	if (strcmp(sess.plugin->info->name, "noop") == 0)
		REQUIRE(output == input);
	else
		REQUIRE(output == 2 * input);

	REQUIRE(vaccel_session_release(&sess) == VACCEL_OK);

	free(lib_path);
}

TEST_CASE("exec_generic", "[ops][exec]")
{
	int ret;
	int input = 10;
	int output = 0;
	struct vaccel_session sess;

	REQUIRE(vaccel_session_init(&sess, 0) == VACCEL_OK);

	vaccel_op_type_t op_type = VACCEL_OP_EXEC;

	char *lib_path = abs_path(BUILD_ROOT, "examples/libmytestlib.so");
	const char function_name[] = "mytestfunc";

	struct vaccel_arg read[4] = {
		{ .argtype = 0, .size = sizeof(uint8_t), .buf = &op_type },
		{ .argtype = 0,
		  .size = sizeof(lib_path),
		  .buf = (void *)lib_path },
		{ .argtype = 0,
		  .size = sizeof(function_name),
		  .buf = (void *)function_name },
		{ .argtype = 0, .size = sizeof(input), .buf = &input }
	};

	struct vaccel_arg write[1] = {
		{ .argtype = 0, .size = sizeof(output), .buf = &output },
	};

	ret = vaccel_genop(&sess, read, 4, write, 1);
	REQUIRE(ret == VACCEL_OK);
	if (strcmp(sess.plugin->info->name, "noop") == 0)
		REQUIRE(output == input);
	else
		REQUIRE(output == 2 * input);

	REQUIRE(vaccel_session_release(&sess) == VACCEL_OK);

	free(lib_path);
}

TEST_CASE("exec_with_resource", "[ops][exec]")
{
	int input = 10;
	int output1 = 0;
	int output2 = 0;
	int ret;
	char *lib_path = abs_path(BUILD_ROOT, "examples/libmytestlib.so");

	size_t len = 0;
	char *buff;

	ret = fs_file_read(lib_path, (void **)&buff, &len);
	REQUIRE(ret == 0);
	REQUIRE(buff);
	REQUIRE(len);

	struct vaccel_resource object1;
	struct vaccel_resource object2;
	REQUIRE(vaccel_resource_init(&object1, lib_path, VACCEL_RESOURCE_LIB) ==
		VACCEL_OK);
	REQUIRE(vaccel_resource_init_from_buf(&object2, (unsigned char *)buff,
					      len, VACCEL_RESOURCE_LIB,
					      "lib.so", false) == VACCEL_OK);

	struct vaccel_session sess1;
	struct vaccel_session sess2;
	REQUIRE(vaccel_session_init(&sess1, 0) == VACCEL_OK);
	REQUIRE(vaccel_session_init(&sess2, 0) == VACCEL_OK);

	REQUIRE(vaccel_resource_register(&object1, &sess1) == VACCEL_OK);
	REQUIRE(vaccel_resource_register(&object2, &sess2) == VACCEL_OK);

	input = 10;
	struct vaccel_arg read[1] = {
		{ .argtype = 0, .size = sizeof(input), .buf = &input }
	};
	struct vaccel_arg write[1] = {
		{ .argtype = 0, .size = sizeof(output1), .buf = &output1 },
	};

	struct vaccel_arg read_2[1] = {
		{ .argtype = 0, .size = sizeof(input), .buf = &input }
	};
	struct vaccel_arg write_2[1] = {
		{ .argtype = 0, .size = sizeof(output2), .buf = &output2 },
	};

	ret = vaccel_exec_with_resource(&sess1, &object1, "mytestfunc", read, 1,
					write, 1);
	REQUIRE(ret == VACCEL_OK);
	if (strcmp(sess1.plugin->info->name, "noop") == 0)
		REQUIRE(output1 == input);
	else
		REQUIRE(output1 == 2 * input);

	ret = vaccel_exec_with_resource(&sess2, &object2, "mytestfunc", read_2,
					1, write_2, 1);
	REQUIRE(ret == VACCEL_OK);
	if (strcmp(sess2.plugin->info->name, "noop") == 0)
		REQUIRE(output2 == input);
	else
		REQUIRE(output2 == 2 * input);

	REQUIRE(vaccel_resource_unregister(&object1, &sess1) == VACCEL_OK);
	REQUIRE(vaccel_resource_unregister(&object2, &sess2) == VACCEL_OK);
	REQUIRE(vaccel_resource_release(&object1) == VACCEL_OK);
	REQUIRE(vaccel_resource_release(&object2) == VACCEL_OK);

	REQUIRE(vaccel_session_release(&sess1) == VACCEL_OK);
	REQUIRE(vaccel_session_release(&sess2) == VACCEL_OK);

	free(buff);
	free(lib_path);
}
