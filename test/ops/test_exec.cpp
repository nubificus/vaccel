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
	sess.id = 0;
	sess.priv = nullptr;
	sess.resources = nullptr;
	sess.hint = 0;

	ret = vaccel_session_init(&sess, VACCEL_PLUGIN_DEBUG);
	REQUIRE(ret == VACCEL_OK);

	struct vaccel_arg read[1] = {
		{ .argtype = 0, .size = sizeof(input), .buf = &input }
	};
	struct vaccel_arg write[1] = {
		{ .argtype = 0, .size = sizeof(output), .buf = &output }
	};

	char *plugin_path = abs_path(BUILD_ROOT, "examples/libmytestlib.so");
	const char function_name[] = "mytestfunc";

	ret = vaccel_exec(&sess, plugin_path, function_name, read, 1, write, 1);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(sess.id);
	REQUIRE(sess.hint == VACCEL_PLUGIN_DEBUG);
	REQUIRE(sess.resources);
	REQUIRE(sess.priv == nullptr);

	ret = vaccel_session_release(&sess);
	REQUIRE(ret == VACCEL_OK);

	free(plugin_path);
}

TEST_CASE("exec_generic", "[ops][exec]")
{
	int ret;

	struct vaccel_session sess;
	sess.id = 0;
	sess.priv = nullptr;
	sess.resources = nullptr;
	sess.hint = 0;

	int input = 10;
	int output = 0;

	ret = vaccel_session_init(&sess, 0);
	REQUIRE(ret == VACCEL_OK);

	vaccel_op_type_t op_type = VACCEL_OP_EXEC;

	char *plugin_path = abs_path(BUILD_ROOT, "examples/libmytestlib.so");
	const char function_name[] = "mytestfunc";

	struct vaccel_arg read[4] = {
		{ .argtype = 0, .size = sizeof(uint8_t), .buf = &op_type },
		{ .argtype = 0,
		  .size = sizeof(plugin_path),
		  .buf = (void *)plugin_path },
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
	printf("output: %d\n", output);
	REQUIRE(sess.id);
	REQUIRE(sess.hint == 0);
	REQUIRE(sess.resources);
	REQUIRE(sess.priv == nullptr);

	ret = vaccel_session_release(&sess);
	REQUIRE(ret == VACCEL_OK);

	free(plugin_path);
}

TEST_CASE("exec_with_resource", "[ops][exec]")
{
	int input;
	int output1 = 0;
	int output2 = 0;
	int ret;
	char *plugin_path = abs_path(BUILD_ROOT, "examples/libmytestlib.so");

	size_t len = 0;
	char *buff;
	ret = fs_file_read(plugin_path, (void **)&buff, &len);
	REQUIRE(ret == 0);
	REQUIRE(buff);
	REQUIRE(len);

	struct vaccel_resource object;
	object.paths = nullptr;

	struct vaccel_resource object2;
	object2.paths = nullptr;

	ret = vaccel_resource_init(&object, plugin_path, VACCEL_RESOURCE_LIB);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(object.paths);
	REQUIRE(object.paths[0]);

	ret = vaccel_resource_init_from_buf(&object2, (unsigned char *)buff,
					    len, VACCEL_RESOURCE_LIB, "lib.so",
					    false);
	REQUIRE(ret == VACCEL_OK);

	struct vaccel_session sess;
	sess.id = 0;
	sess.priv = nullptr;
	sess.resources = nullptr;
	sess.hint = 1;

	struct vaccel_session sess2;
	sess2.id = 0;
	sess2.priv = nullptr;
	sess2.resources = nullptr;
	sess2.hint = 1;

	ret = vaccel_session_init(&sess, 0);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(sess.id);
	REQUIRE(sess.hint == 0);
	REQUIRE(sess.resources);
	REQUIRE(sess.priv == nullptr);

	ret = vaccel_session_init(&sess2, 0);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(sess2.id);
	REQUIRE(sess2.hint == 0);
	REQUIRE(sess2.resources);
	REQUIRE(sess2.priv == nullptr);

	ret = vaccel_resource_register(&object, &sess);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(sess.id);
	REQUIRE(sess.hint == 0);
	REQUIRE_FALSE(list_empty(&sess.resources->registered[object.type]));
	REQUIRE(sess.priv == nullptr);
	REQUIRE(object.blobs);

	ret = vaccel_resource_register(&object2, &sess2);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(sess2.id);
	REQUIRE(sess2.hint == 0);
	REQUIRE_FALSE(list_empty(&sess2.resources->registered[object2.type]));
	REQUIRE(sess2.priv == nullptr);
	REQUIRE(object2.blobs);

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

	ret = vaccel_exec_with_resource(&sess, &object, "mytestfunc", read, 1,
					write, 1);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(sess.id);
	REQUIRE(sess.hint == 0);
	REQUIRE_FALSE(list_empty(&sess.resources->registered[object.type]));
	REQUIRE(sess.priv == nullptr);
	REQUIRE(output1);

	ret = vaccel_exec_with_resource(&sess2, &object2, "mytestfunc", read_2,
					1, write_2, 1);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(sess2.id);
	REQUIRE(sess2.hint == 0);
	REQUIRE_FALSE(list_empty(&sess2.resources->registered[object2.type]));
	REQUIRE(sess2.priv == nullptr);
	REQUIRE(output2);

	ret = vaccel_resource_unregister(&object, &sess);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(sess.id);
	REQUIRE(sess.hint == 0);
	REQUIRE(list_empty(&sess.resources->registered[object.type]));
	REQUIRE(sess.priv == nullptr);

	ret = vaccel_resource_unregister(&object2, &sess2);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(sess2.id);
	REQUIRE(sess2.hint == 0);
	REQUIRE(list_empty(&sess2.resources->registered[object2.type]));
	REQUIRE(sess2.priv == nullptr);

	ret = vaccel_resource_release(&object);
	REQUIRE(ret == VACCEL_OK);

	ret = vaccel_resource_release(&object2);
	REQUIRE(ret == VACCEL_OK);

	ret = vaccel_session_release(&sess);
	REQUIRE(ret == VACCEL_OK);

	ret = vaccel_session_release(&sess2);
	REQUIRE(ret == VACCEL_OK);

	free(buff);
	free(plugin_path);
}
