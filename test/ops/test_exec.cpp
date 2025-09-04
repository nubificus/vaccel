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
	int32_t input = 10;
	int32_t output = 0;
	struct vaccel_session sess;

	REQUIRE(vaccel_session_init(&sess, 0) == VACCEL_OK);

	struct vaccel_arg_array read_args;
	struct vaccel_arg_array write_args;
	REQUIRE(vaccel_arg_array_init(&read_args, 1) == VACCEL_OK);
	REQUIRE(vaccel_arg_array_init(&write_args, 1) == VACCEL_OK);

	REQUIRE(vaccel_arg_array_add_int32(&read_args, &input) == VACCEL_OK);
	REQUIRE(vaccel_arg_array_add_int32(&write_args, &output) == VACCEL_OK);

	char *lib_path = abs_path(BUILD_ROOT, "examples/libmytestlib.so");
	const char function_name[] = "mytestfunc";

	ret = vaccel_exec(&sess, lib_path, function_name, read_args.args,
			  read_args.count, write_args.args, write_args.count);
	REQUIRE(ret == VACCEL_OK);
	if (strcmp(sess.plugin->info->name, "noop") == 0)
		REQUIRE(output == input);
	else
		REQUIRE(output == 2 * input);

	REQUIRE(vaccel_session_release(&sess) == VACCEL_OK);
	REQUIRE(vaccel_arg_array_release(&read_args) == VACCEL_OK);
	REQUIRE(vaccel_arg_array_release(&write_args) == VACCEL_OK);
	free(lib_path);
}

TEST_CASE("exec_generic", "[ops][exec]")
{
	int ret;
	int32_t input = 10;
	int32_t output = 0;
	struct vaccel_session sess;

	REQUIRE(vaccel_session_init(&sess, 0) == VACCEL_OK);

	char *lib_path = abs_path(BUILD_ROOT, "examples/libmytestlib.so");
	const char function_name[] = "mytestfunc";

	struct vaccel_arg_array read_args;
	struct vaccel_arg_array write_args;
	REQUIRE(vaccel_arg_array_init(&read_args, 4) == VACCEL_OK);
	REQUIRE(vaccel_arg_array_init(&write_args, 1) == VACCEL_OK);

	const auto op_type = (uint8_t)VACCEL_OP_EXEC;
	REQUIRE(vaccel_arg_array_add_uint8(&read_args, (uint8_t *)&op_type) ==
		VACCEL_OK);
	REQUIRE(vaccel_arg_array_add_string(&read_args, lib_path) == VACCEL_OK);
	REQUIRE(vaccel_arg_array_add_string(
			&read_args, (char *)function_name) == VACCEL_OK);
	REQUIRE(vaccel_arg_array_add_int32(&read_args, &input) == VACCEL_OK);
	REQUIRE(vaccel_arg_array_add_int32(&write_args, &output) == VACCEL_OK);

	ret = vaccel_genop(&sess, read_args.args, read_args.count,
			   write_args.args, write_args.count);
	REQUIRE(ret == VACCEL_OK);
	if (strcmp(sess.plugin->info->name, "noop") == 0)
		REQUIRE(output == input);
	else
		REQUIRE(output == 2 * input);

	REQUIRE(vaccel_session_release(&sess) == VACCEL_OK);
	REQUIRE(vaccel_arg_array_release(&read_args) == VACCEL_OK);
	REQUIRE(vaccel_arg_array_release(&write_args) == VACCEL_OK);
	free(lib_path);
}

TEST_CASE("exec_with_resource", "[ops][exec]")
{
	int32_t input = 10;
	int32_t output1 = 0;
	int32_t output2 = 0;
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

	struct vaccel_arg_array read_args;
	struct vaccel_arg_array write_args;
	REQUIRE(vaccel_arg_array_init(&read_args, 1) == VACCEL_OK);
	REQUIRE(vaccel_arg_array_init(&write_args, 1) == VACCEL_OK);

	REQUIRE(vaccel_arg_array_add_int32(&read_args, &input) == VACCEL_OK);
	REQUIRE(vaccel_arg_array_add_int32(&write_args, &output1) == VACCEL_OK);

	ret = vaccel_exec_with_resource(&sess1, &object1, "mytestfunc",
					read_args.args, read_args.count,
					write_args.args, write_args.count);
	REQUIRE(ret == VACCEL_OK);
	if (strcmp(sess1.plugin->info->name, "noop") == 0)
		REQUIRE(output1 == input);
	else
		REQUIRE(output1 == 2 * input);

	vaccel_arg_array_clear(&write_args);
	REQUIRE(vaccel_arg_array_add_int32(&write_args, &output2) == VACCEL_OK);

	ret = vaccel_exec_with_resource(&sess2, &object2, "mytestfunc",
					read_args.args, read_args.count,
					write_args.args, write_args.count);
	REQUIRE(ret == VACCEL_OK);
	if (strcmp(sess2.plugin->info->name, "noop") == 0)
		REQUIRE(output2 == input);
	else
		REQUIRE(output2 == 2 * input);

	REQUIRE(vaccel_resource_unregister(&object1, &sess1) == VACCEL_OK);
	REQUIRE(vaccel_resource_unregister(&object2, &sess2) == VACCEL_OK);
	REQUIRE(vaccel_resource_release(&object1) == VACCEL_OK);
	REQUIRE(vaccel_resource_release(&object2) == VACCEL_OK);

	REQUIRE(vaccel_arg_array_release(&read_args) == VACCEL_OK);
	REQUIRE(vaccel_arg_array_release(&write_args) == VACCEL_OK);
	REQUIRE(vaccel_session_release(&sess1) == VACCEL_OK);
	REQUIRE(vaccel_session_release(&sess2) == VACCEL_OK);

	free(buff);
	free(lib_path);
}
