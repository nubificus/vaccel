// SPDX-License-Identifier: Apache-2.0

/*
 * TensorFlow Saved Model from Memory Test
 *
 * The code below tests the creation and registration of a TensorFlow saved
 * model from memory.
 *
 */

#include <catch.hpp>
#include <utils.hpp>

#include "utils.h"
#include <fcntl.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <vaccel.h>

TEST_CASE("tf_saved_model_from_memory", "[resources_tf_saved_model]")
{
	int ret;
	struct vaccel_resource model;
	char *path = abs_path(SOURCE_ROOT, "examples/models/tf/lstm2/");
	char path1[200] = { '\0' };
	char path2[200] = { '\0' };

	sprintf(path1, "%s%s", path, "saved_model.pb");
	sprintf(path2, "%s%s", path, "variables/variables.index");

	char *paths[] = { path1, path2, path2 };

	ret = vaccel_resource_new_multi(&model, paths, VACCEL_FILE_DATA, 3);
	REQUIRE(ret == VACCEL_OK);

	vaccel_info("Registered new resource: %ld", model.id);

	struct vaccel_session sess;
	ret = vaccel_sess_init(&sess, 0);
	REQUIRE(ret == VACCEL_OK);

	vaccel_info("Registering model %ld with session %u", model.id,
		    sess.session_id);

	ret = vaccel_resource_register(&sess, &model);
	REQUIRE(ret == VACCEL_OK);

	ret = vaccel_resource_unregister(&sess, &model);
	REQUIRE(ret == VACCEL_OK);

	ret = vaccel_resource_destroy(&model);
	REQUIRE(ret == VACCEL_OK);

	ret = vaccel_sess_free(&sess);
	REQUIRE(ret == VACCEL_OK);

	free(path);
}

TEST_CASE("tf_saved_model_from_file", "[resources_tf_saved_model]")
{
	int ret;
	char *path = abs_path(SOURCE_ROOT,
			      "examples/models/tf/lstm2/saved_model.pb");

	struct vaccel_resource model;

	ret = vaccel_resource_new(&model, path, VACCEL_FILE_DATA);
	REQUIRE(ret == VACCEL_OK);

	vaccel_info("Registered new resource: %ld", model.id);

	struct vaccel_session sess;
	ret = vaccel_sess_init(&sess, 0);
	REQUIRE(ret == VACCEL_OK);

	ret = vaccel_resource_register(&sess, &model);
	REQUIRE(ret == VACCEL_OK);

	ret = vaccel_resource_unregister(&sess, &model);
	REQUIRE(ret == VACCEL_OK);

	ret = vaccel_resource_destroy(&model);
	REQUIRE(ret == VACCEL_OK);

	ret = vaccel_sess_free(&sess);
	REQUIRE(ret == VACCEL_OK);

	free(path);
}
