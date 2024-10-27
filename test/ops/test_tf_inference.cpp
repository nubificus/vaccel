// SPDX-License-Identifier: Apache-2.0

/*
 * TensorFlow Inference Test
 *
 * The code below performs a TensorFlow inference test.
 *
 */

#include <catch.hpp>
#include <utils.hpp>

#include <cinttypes>
#include <cstdio>
#include <cstdlib>
#include <vaccel.h>

#define min(a, b) ((a) < (b) ? (a) : (b))

TEST_CASE("tf_inference", "[ops_tf]")
{
	struct vaccel_session vsess;
	struct vaccel_resource model;
	struct vaccel_tf_status status;
	int ret;
	char *model_path = abs_path(SOURCE_ROOT, "examples/models/tf/lstm2");

	vsess.id = 0;
	vsess.resources = nullptr;
	vsess.hint = 0;
	vsess.priv = nullptr;

	model.paths = nullptr;

	ret = vaccel_resource_init(&model, model_path, VACCEL_RESOURCE_MODEL);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE_FALSE(model.paths == nullptr);
	REQUIRE_FALSE(model.paths[0] == nullptr);
	INFO("model.paths[0]: " << model.paths[0]);

	ret = vaccel_session_init(&vsess, 0);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE_FALSE(vsess.id == 0);
	REQUIRE_FALSE(vsess.resources == nullptr);
	REQUIRE(vsess.hint == 0);
	REQUIRE(vsess.priv == nullptr);

	printf("Initialized vAccel session %" PRId64 "\n", vsess.id);

	ret = vaccel_resource_register(&model, &vsess);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE_FALSE(vsess.id == 0);
	REQUIRE(vsess.hint == 0);
	REQUIRE_FALSE(list_empty(&vsess.resources->registered[model.type]));
	REQUIRE(vsess.priv == nullptr);
	REQUIRE_FALSE(model.files == nullptr);
	REQUIRE_FALSE(model.files[0] == nullptr);

	ret = vaccel_tf_session_load(&vsess, &model, &status);
	REQUIRE(ret == VACCEL_OK);

	if (status.message != nullptr)
		free((char *)status.message);

	struct vaccel_tf_buffer run_options = { .data = nullptr, .size = 0 };
	const char *in_node_name = "serving_default_input_1";

	struct vaccel_tf_node in_node = {
		.name = const_cast<char *>(in_node_name), .id = 0
	};

	int64_t dims[] = { 1, 30 };
	float data[30];
	for (float &i : data)
		i = 1.00;

	struct vaccel_tf_tensor *in =
		vaccel_tf_tensor_new(2, dims, VACCEL_TF_FLOAT);
	REQUIRE(in);

	in->data = data;
	in->size = sizeof(float) * 30;

	const char *out_node_name = "StatefulPartitionedCall";

	struct vaccel_tf_node out_node = {
		.name = const_cast<char *>(out_node_name), .id = 0
	};
	struct vaccel_tf_tensor *out;

	ret = vaccel_tf_session_run(&vsess, &model, &run_options, &in_node, &in,
				    1, &out_node, &out, 1, &status);
	REQUIRE(ret == VACCEL_OK);

	if (status.message != nullptr)
		free((char *)status.message);

	printf("Success!\n");
	printf("Output tensor => type:%u nr_dims:%u\n", out->data_type,
	       out->nr_dims);
	for (int i = 0; i < out->nr_dims; ++i)
		printf("dim[%d]: %" PRId64 "\n", i, out->dims[i]);

	printf("Result Tensor :\n");
	auto *offsets = (float *)out->data;
	for (unsigned int i = 0; i < min(10, out->size / sizeof(float)); ++i)
		printf("%f\n", offsets[i]);

	ret = vaccel_tf_tensor_destroy(in);
	REQUIRE(ret == VACCEL_OK);

	ret = vaccel_tf_tensor_destroy(out);
	REQUIRE(ret == VACCEL_OK);

	ret = vaccel_tf_session_delete(&vsess, &model, &status);
	REQUIRE(ret == VACCEL_OK);

	if (status.message != nullptr)
		free((char *)status.message);

	ret = vaccel_resource_unregister(&model, &vsess);
	REQUIRE(ret == VACCEL_OK);

	ret = vaccel_session_free(&vsess);
	REQUIRE(ret == VACCEL_OK);

	ret = vaccel_resource_release(&model);
	REQUIRE(ret == VACCEL_OK);

	free(model_path);
}
