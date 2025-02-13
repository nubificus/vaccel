// SPDX-License-Identifier: Apache-2.0

/*
 * The code below performs unit testing for Tensorflow Lite operations.
 *
 */

#include "utils.hpp"
#include "vaccel.h"
#include <catch.hpp>
#include <cstdint>
#include <cstdio>
#include <cstdlib>

TEST_CASE("tflite_tensor_init", "[ops][tflite]")
{
	int ret;
	int32_t dims[] = { 1, 30 };
	struct vaccel_tflite_tensor tensor;

	SECTION("success")
	{
		ret = vaccel_tflite_tensor_init(&tensor, 2, dims,
						VACCEL_TFLITE_FLOAT32);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(tensor.data_type == VACCEL_TFLITE_FLOAT32);
		REQUIRE(tensor.nr_dims == 2);
		REQUIRE(tensor.dims[0] == 1);
		REQUIRE(tensor.dims[1] == 30);
		REQUIRE(tensor.data == nullptr);
		REQUIRE(tensor.size == 0);
		REQUIRE_FALSE(tensor.owned);

		REQUIRE(vaccel_tflite_tensor_release(&tensor) == VACCEL_OK);
	}

	SECTION("invalid arguments")
	{
		ret = vaccel_tflite_tensor_init(nullptr, 2, dims,
						VACCEL_TFLITE_FLOAT32);
		REQUIRE(ret == VACCEL_EINVAL);

		ret = vaccel_tflite_tensor_init(&tensor, 0, dims,
						VACCEL_TFLITE_FLOAT32);
		REQUIRE(ret == VACCEL_EINVAL);
	}
}

TEST_CASE("tflite_tensor_release", "[ops][tflite]")
{
	int ret;
	int32_t dims[] = { 1, 30 };
	struct vaccel_tflite_tensor tensor;

	SECTION("success")
	{
		REQUIRE(vaccel_tflite_tensor_init(&tensor, 2, dims,
						  VACCEL_TFLITE_FLOAT32) ==
			VACCEL_OK);
		// dummy values
		tensor.size = 1;
		tensor.owned = true;

		ret = vaccel_tflite_tensor_release(&tensor);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(tensor.nr_dims == 0);
		REQUIRE(tensor.dims == nullptr);
		REQUIRE(tensor.data == nullptr);
		REQUIRE(tensor.size == 0);
		REQUIRE_FALSE(tensor.owned);
	}

	SECTION("invalid arguments")
	{
		ret = vaccel_tflite_tensor_release(nullptr);
		REQUIRE(ret == VACCEL_EINVAL);
	}
}

TEST_CASE("tflite_tensor_new", "[ops][tflite]")
{
	int ret;
	int32_t dims[] = { 1, 30 };
	struct vaccel_tflite_tensor *tensor = nullptr;

	SECTION("success")
	{
		ret = vaccel_tflite_tensor_new(&tensor, 2, dims,
					       VACCEL_TFLITE_FLOAT32);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(tensor != nullptr);

		REQUIRE(vaccel_tflite_tensor_delete(tensor) == VACCEL_OK);
	}

	SECTION("invalid arguments")
	{
		ret = vaccel_tflite_tensor_new(nullptr, 2, dims,
					       VACCEL_TFLITE_FLOAT32);
		REQUIRE(ret == VACCEL_EINVAL);

		ret = vaccel_tflite_tensor_new(&tensor, 0, dims,
					       VACCEL_TFLITE_FLOAT32);
		REQUIRE(ret == VACCEL_EINVAL);
	}
}

TEST_CASE("tflite_tensor_allocate", "[ops][tflite]")
{
	int ret;
	int32_t dims[] = { 1, 30 };
	struct vaccel_tflite_tensor *tensor = nullptr;

	SECTION("success")
	{
		ret = vaccel_tflite_tensor_allocate(&tensor, 2, dims,
						    VACCEL_TFLITE_FLOAT32, 1);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(tensor != nullptr);
		REQUIRE(tensor->data);
		REQUIRE(tensor->size == 1);
		REQUIRE(tensor->owned);

		REQUIRE(vaccel_tflite_tensor_delete(tensor) == VACCEL_OK);
	}

	SECTION("success without data allocation")
	{
		ret = vaccel_tflite_tensor_allocate(&tensor, 2, dims,
						    VACCEL_TFLITE_FLOAT32, 0);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(tensor != nullptr);
		REQUIRE(tensor->data == nullptr);
		REQUIRE(tensor->size == 0);
		REQUIRE_FALSE(tensor->owned);

		REQUIRE(vaccel_tflite_tensor_delete(tensor) == VACCEL_OK);
	}

	SECTION("invalid arguments")
	{
		ret = vaccel_tflite_tensor_allocate(nullptr, 2, dims,
						    VACCEL_TFLITE_FLOAT32, 1);
		REQUIRE(ret == VACCEL_EINVAL);

		ret = vaccel_tflite_tensor_allocate(&tensor, 0, dims,
						    VACCEL_TFLITE_FLOAT32, 1);
		REQUIRE(ret == VACCEL_EINVAL);
	}
}

TEST_CASE("tflite_tensor_delete", "[ops][tflite]")
{
	int ret;
	int32_t dims[] = { 1, 30 };
	struct vaccel_tflite_tensor *tensor = nullptr;

	SECTION("success")
	{
		REQUIRE(vaccel_tflite_tensor_new(&tensor, 2, dims,
						 VACCEL_TFLITE_FLOAT32) ==
			VACCEL_OK);

		ret = vaccel_tflite_tensor_delete(tensor);
		REQUIRE(ret == VACCEL_OK);
	}

	SECTION("invalid arguments")
	{
		ret = vaccel_tflite_tensor_delete(nullptr);
		REQUIRE(ret == VACCEL_EINVAL);
	}
}

TEST_CASE("tflite_tensor_set_data", "[ops][tflite]")
{
	int ret;
	int32_t dims[] = { 1, 30 };
	struct vaccel_tflite_tensor tensor;

	REQUIRE(vaccel_tflite_tensor_init(&tensor, 2, dims,
					  VACCEL_TFLITE_FLOAT32) == VACCEL_OK);

	SECTION("success")
	{
		ret = vaccel_tflite_tensor_set_data(&tensor, dims,
						    sizeof(dims));
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(tensor.data == dims);
		REQUIRE(tensor.size == sizeof(dims));
		REQUIRE_FALSE(tensor.owned);

		// ensure data is not freed on release
		REQUIRE(vaccel_tflite_tensor_release(&tensor) == VACCEL_OK);
	}

	SECTION("invalid arguments")
	{
		ret = vaccel_tflite_tensor_set_data(nullptr, dims,
						    sizeof(dims));
		REQUIRE(ret == VACCEL_EINVAL);
	}

	REQUIRE(vaccel_tflite_tensor_release(&tensor) == VACCEL_OK);
}

TEST_CASE("tflite_tensor_take_data", "[ops][tflite]")
{
	int ret;
	int32_t dims[] = { 1, 30 };
	struct vaccel_tflite_tensor *tensor = nullptr;
	void *data;
	size_t size;

	REQUIRE(vaccel_tflite_tensor_allocate(&tensor, 2, dims,
					      VACCEL_TFLITE_FLOAT32,
					      1) == VACCEL_OK);

	SECTION("success")
	{
		ret = vaccel_tflite_tensor_take_data(tensor, &data, &size);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(tensor->data == nullptr);
		REQUIRE(tensor->size == 0);
		REQUIRE_FALSE(tensor->owned);

		REQUIRE(data);
		REQUIRE(size == 1);

		free(data);
		// ensure data is not freed on release
		REQUIRE(vaccel_tflite_tensor_release(tensor) == VACCEL_OK);
	}

	SECTION("invalid arguments")
	{
		ret = vaccel_tflite_tensor_take_data(nullptr, &data, &size);
		REQUIRE(ret == VACCEL_EINVAL);

		ret = vaccel_tflite_tensor_take_data(tensor, nullptr, &size);
		REQUIRE(ret == VACCEL_EINVAL);

		ret = vaccel_tflite_tensor_take_data(tensor, &data, nullptr);
		REQUIRE(ret == VACCEL_EINVAL);
	}

	REQUIRE(vaccel_tflite_tensor_delete(tensor) == VACCEL_OK);
}

TEST_CASE("tflite_tensor_ops", "[ops][tflite]")
{
	int32_t dims[] = { 1, 30 };

	// allocate data with dummy size
	struct vaccel_tflite_tensor *tensor = nullptr;
	int ret = vaccel_tflite_tensor_allocate(&tensor, 2, dims,
						VACCEL_TFLITE_FLOAT32, 8);
	REQUIRE(ret == VACCEL_OK);

	// take data
	void *data;
	size_t size;
	ret = vaccel_tflite_tensor_take_data(tensor, &data, &size);
	REQUIRE(ret == VACCEL_OK);

	// re-set other dummy data
	ret = vaccel_tflite_tensor_set_data(tensor, dims, sizeof(dims));
	REQUIRE(ret == VACCEL_OK);

	ret = vaccel_tflite_tensor_delete(tensor);
	REQUIRE(ret == VACCEL_OK);

	free(data);
}

TEST_CASE("tflite_inference", "[ops][tflite]")
{
	struct vaccel_session vsess;
	struct vaccel_resource model;
	int ret;
	char *model_path =
		abs_path(SOURCE_ROOT, "examples/models/tf/lstm2.tflite");

	REQUIRE(vaccel_resource_init(&model, model_path,
				     VACCEL_RESOURCE_MODEL) == VACCEL_OK);
	REQUIRE(vaccel_session_init(&vsess, 0) == VACCEL_OK);
	REQUIRE(vaccel_resource_register(&model, &vsess) == VACCEL_OK);

	ret = vaccel_tflite_session_load(&vsess, &model);
	REQUIRE(ret == VACCEL_OK);

	int32_t dims[] = { 1, 30 };
	float data[30];
	for (float &i : data)
		i = 1.00;

	struct vaccel_tflite_tensor *in;
	REQUIRE(vaccel_tflite_tensor_new(&in, 2, dims, VACCEL_TFLITE_FLOAT32) ==
		VACCEL_OK);
	REQUIRE(vaccel_tflite_tensor_set_data(in, data, sizeof(data)) ==
		VACCEL_OK);

	uint8_t status;
	struct vaccel_tflite_tensor *out;
	ret = vaccel_tflite_session_run(&vsess, &model, &in, 1, &out, 1,
					&status);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(out->data_type == in->data_type);
	REQUIRE(out->nr_dims == in->nr_dims);
	REQUIRE(out->dims[0] == in->dims[0]);
	REQUIRE(out->dims[1] == in->dims[1]);
	REQUIRE(out->size == in->size);
	for (size_t i = 0; i < out->size / sizeof(float); i++)
		REQUIRE(((float *)out->data)[i] == ((float *)in->data)[i]);
	REQUIRE(out->owned);
	REQUIRE(status == 0);

	REQUIRE(vaccel_tflite_tensor_delete(in) == VACCEL_OK);
	REQUIRE(vaccel_tflite_tensor_delete(out) == VACCEL_OK);

	ret = vaccel_tflite_session_delete(&vsess, &model);
	REQUIRE(ret == VACCEL_OK);

	REQUIRE(vaccel_resource_unregister(&model, &vsess) == VACCEL_OK);
	REQUIRE(vaccel_session_release(&vsess) == VACCEL_OK);
	REQUIRE(vaccel_resource_release(&model) == VACCEL_OK);

	free(model_path);
}
