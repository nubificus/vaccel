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

TEST_CASE("tflite_tensor_ops", "[ops_tflite]")
{
	int32_t dims[] = { 1, 30 };

	// no data allocated
	struct vaccel_tflite_tensor *tensor1 = vaccel_tflite_tensor_allocate(
		2, dims, VACCEL_TFLITE_FLOAT32, 0);
	REQUIRE(tensor1);
	REQUIRE(tensor1->dims);
	REQUIRE(tensor1->data == nullptr);
	REQUIRE(tensor1->size == 0);

	int ret = vaccel_tflite_tensor_destroy(tensor1);
	REQUIRE(ret == VACCEL_OK);

	// set dummy data
	struct vaccel_tflite_tensor *tensor2 = vaccel_tflite_tensor_allocate(
		2, dims, VACCEL_TFLITE_FLOAT32, 0);
	REQUIRE(tensor2);
	REQUIRE(tensor2->data == nullptr);
	REQUIRE(tensor2->size == 0);

	ret = vaccel_tflite_tensor_set_data(tensor2, (void *)dims,
					    sizeof(int32_t) * 2);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(tensor2->data == dims);
	REQUIRE(tensor2->size == sizeof(int32_t) * 2);

	ret = vaccel_tflite_tensor_destroy(tensor2);
	REQUIRE(ret == VACCEL_OK);

	// no data allocated + no dims
	struct vaccel_tflite_tensor *tensor3 = vaccel_tflite_tensor_allocate(
		2, nullptr, VACCEL_TFLITE_FLOAT32, 0);
	REQUIRE(tensor3);
	REQUIRE(tensor3->dims);
	REQUIRE(tensor3->data == nullptr);
	REQUIRE(tensor3->size == 0);

	ret = vaccel_tflite_tensor_destroy(tensor3);
	REQUIRE(ret == VACCEL_OK);

	// allocate data with dummy size
	struct vaccel_tflite_tensor *tensor4 = vaccel_tflite_tensor_allocate(
		2, dims, VACCEL_TFLITE_FLOAT32, 8);
	REQUIRE(tensor4);
	REQUIRE(tensor4->data);
	REQUIRE(tensor4->size == 8);
	REQUIRE(tensor4->owned == true);

	// re-set dummy data
	ret = vaccel_tflite_tensor_set_data(tensor4, (void *)dims,
					    sizeof(int32_t) * 2);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(tensor4->data == dims);
	REQUIRE(tensor4->size == sizeof(int32_t) * 2);

	ret = vaccel_tflite_tensor_destroy(tensor4);
	REQUIRE(ret == VACCEL_OK);

	// null tensor ops
	ret = vaccel_tflite_tensor_set_data(nullptr, nullptr, 0);
	REQUIRE(ret == VACCEL_EINVAL);

	void *d = vaccel_tflite_tensor_get_data(nullptr);
	REQUIRE(d == nullptr);

	ret = vaccel_tflite_tensor_destroy(nullptr);
	REQUIRE(ret == VACCEL_EINVAL);
}

TEST_CASE("tflite_inference", "[ops_tflite]")
{
	struct vaccel_session vsess;
	struct vaccel_single_model model;
	int ret;
	char *model_path =
		abs_path(SOURCE_ROOT, "examples/models/tf/lstm2.tflite");

	vsess.session_id = 0;
	vsess.resources = nullptr;
	vsess.hint = 0;
	vsess.priv = nullptr;

	model.resource = nullptr;
	model.path = nullptr;
	model.plugin_data = nullptr;

	ret = vaccel_single_model_set_path(&model, model_path);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(model.resource == NULL);
	REQUIRE_FALSE(model.path == nullptr);
	REQUIRE(model.plugin_data == nullptr);
	INFO("model.path: " << model.path);

	ret = vaccel_single_model_register(&model);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE_FALSE(model.path == nullptr);
	REQUIRE(model.plugin_data == nullptr);
	REQUIRE_FALSE(model.resource == NULL);

	ret = vaccel_sess_init(&vsess, 0);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE_FALSE(vsess.session_id == 0);
	REQUIRE_FALSE(vsess.resources == nullptr);
	REQUIRE(vsess.hint == 0);
	REQUIRE(vsess.priv == nullptr);

	printf("Initialized vAccel session %u\n", vsess.session_id);

	ret = vaccel_sess_register(&vsess, model.resource);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE_FALSE(vsess.session_id == 0);
	REQUIRE(vsess.hint == 0);
	REQUIRE_FALSE(
		list_empty(&vsess.resources->registered[model.resource->type]));
	REQUIRE(vsess.priv == nullptr);

	ret = vaccel_tflite_session_load(&vsess, &model);
	REQUIRE(ret == VACCEL_OK);

	int32_t dims[] = { 1, 30 };
	float data[30];
	for (float &i : data)
		i = 1.00;

	struct vaccel_tflite_tensor *in =
		vaccel_tflite_tensor_new(2, dims, VACCEL_TFLITE_FLOAT32);
	REQUIRE(in);

	in->data = data;
	in->size = sizeof(float) * 30;

	uint8_t status;
	struct vaccel_tflite_tensor *out;

	ret = vaccel_tflite_session_run(&vsess, &model, &in, 1, &out, 1,
					&status);
	REQUIRE(ret == VACCEL_OK);

	printf("Success!\n");
	printf("Output tensor => type:%u nr_dims:%u\n", out->data_type,
	       out->nr_dims);
	for (int i = 0; i < out->nr_dims; ++i)
		printf("dim[%d]: %" PRId32 "\n", i, out->dims[i]);

	printf("Result Tensor :\n");
	auto *offsets = (float *)out->data;
	for (unsigned int i = 0; i < min(10, out->size / sizeof(float)); ++i)
		printf("%f\n", offsets[i]);

	ret = vaccel_tflite_tensor_destroy(in);
	REQUIRE(ret == VACCEL_OK);

	ret = vaccel_tflite_tensor_destroy(out);
	REQUIRE(ret == VACCEL_OK);

	ret = vaccel_tflite_session_delete(&vsess, &model);
	REQUIRE(ret == VACCEL_OK);

	ret = vaccel_sess_unregister(&vsess, model.resource);
	REQUIRE(ret == VACCEL_OK);

	ret = vaccel_sess_free(&vsess);
	REQUIRE(ret == VACCEL_OK);

	ret = vaccel_single_model_destroy(&model);
	REQUIRE(ret == VACCEL_OK);

	free(model_path);
}
