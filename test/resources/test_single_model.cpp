// SPDX-License-Identifier: Apache-2.0

/*
 * Single Model from Memory Test
 *
 * The code below tests the creation and registration of a TensorFlow saved
 * model from memory.
 *
 */

#include <catch.hpp>
#include <utils.hpp>

#include "utils.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <vaccel.h>

TEST_CASE("single_model_from_memory", "[resources_single_model]")
{
	int ret;
	char *path = abs_path(SOURCE_ROOT, "examples/models/tf/lstm2.tflite");

	struct vaccel_resource model;

	size_t len;
	unsigned char *ptr;
	ret = read_file(path, (void **)&ptr, &len);
	REQUIRE(ret == 0);
	REQUIRE(len);

	ret = vaccel_resource_init_from_buf(&model, ptr, len,
					    VACCEL_RESOURCE_MODEL, nullptr);
	REQUIRE(ret == VACCEL_OK);

	vaccel_info("Registered new resource: %ld", model.id);

	struct vaccel_session sess;
	ret = vaccel_session_init(&sess, 0);
	REQUIRE(ret == VACCEL_OK);

	vaccel_info("Registering model %ld with session %u", model.id,
		    sess.session_id);

	ret = vaccel_resource_register(&model, &sess);
	REQUIRE(ret == VACCEL_OK);

	ret = vaccel_resource_unregister(&model, &sess);
	REQUIRE(ret == VACCEL_OK);

	ret = vaccel_resource_release(&model);
	REQUIRE(ret == VACCEL_OK);

	ret = vaccel_session_free(&sess);
	REQUIRE(ret == VACCEL_OK);

	free(ptr);
	free(path);
}

TEST_CASE("single_model_from_file", "[resources_single_model]")
{
	int ret;
	char *path = abs_path(SOURCE_ROOT, "examples/models/tf/lstm2.tflite");

	struct vaccel_resource model;

	ret = vaccel_resource_init(&model, path, VACCEL_RESOURCE_MODEL);
	REQUIRE(ret == VACCEL_OK);

	vaccel_info("Registered new resource: %ld", model.id);

	struct vaccel_session sess;
	ret = vaccel_session_init(&sess, 0);
	REQUIRE(ret == VACCEL_OK);

	ret = vaccel_resource_register(&model, &sess);
	REQUIRE(ret == VACCEL_OK);

	ret = vaccel_resource_unregister(&model, &sess);
	REQUIRE(ret == VACCEL_OK);

	ret = vaccel_resource_release(&model);
	REQUIRE(ret == VACCEL_OK);

	ret = vaccel_session_free(&sess);
	REQUIRE(ret == VACCEL_OK);

	free(path);
}
