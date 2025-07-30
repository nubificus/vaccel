// SPDX-License-Identifier: Apache-2.0

/*
 * The code below performs unit testing for Torch operations.
 *
 */

#include "utils.hpp"
#include "vaccel.h"
#include <catch.hpp>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

TEST_CASE("torch_buffer_init", "[ops][torch]")
{
	int ret;
	char data_str[] = "test";
	size_t const size = strlen(data_str) + 1;
	struct vaccel_torch_buffer buffer;

	SECTION("success")
	{
		char *data = strdup(data_str);

		ret = vaccel_torch_buffer_init(&buffer, data, size);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(buffer.data == data);
		REQUIRE(buffer.size == size);

		REQUIRE(vaccel_torch_buffer_release(&buffer) == VACCEL_OK);
	}

	SECTION("invalid arguments")
	{
		ret = vaccel_torch_buffer_init(nullptr, data_str, size);
		REQUIRE(ret == VACCEL_EINVAL);
	}
}

TEST_CASE("torch_buffer_release", "[ops][torch]")
{
	int ret;
	char data_str[] = "test";
	size_t const size = strlen(data_str) + 1;
	struct vaccel_torch_buffer buffer;

	SECTION("success")
	{
		char *data = strdup(data_str);

		REQUIRE(vaccel_torch_buffer_init(&buffer, data, size) ==
			VACCEL_OK);

		ret = vaccel_torch_buffer_release(&buffer);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(buffer.data == nullptr);
		REQUIRE(buffer.size == 0);
	}

	SECTION("invalid arguments")
	{
		ret = vaccel_torch_buffer_release(nullptr);
		REQUIRE(ret == VACCEL_EINVAL);
	}
}

TEST_CASE("torch_buffer_new", "[ops][torch]")
{
	int ret;
	char data_str[] = "test";
	size_t const size = strlen(data_str) + 1;
	struct vaccel_torch_buffer *buffer = nullptr;

	SECTION("success")
	{
		char *data = strdup(data_str);

		ret = vaccel_torch_buffer_new(&buffer, data, size);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(buffer != nullptr);

		REQUIRE(vaccel_torch_buffer_delete(buffer) == VACCEL_OK);
	}

	SECTION("invalid arguments")
	{
		ret = vaccel_torch_buffer_new(nullptr, data_str, size);
		REQUIRE(ret == VACCEL_EINVAL);
	}
}

TEST_CASE("torch_buffer_delete", "[ops][torch]")
{
	int ret;
	char data_str[] = "test";
	size_t const size = strlen(data_str) + 1;
	struct vaccel_torch_buffer *buffer = nullptr;

	SECTION("success")
	{
		char *data = strdup(data_str);

		REQUIRE(vaccel_torch_buffer_new(&buffer, data, size) ==
			VACCEL_OK);

		ret = vaccel_torch_buffer_delete(buffer);
		REQUIRE(ret == VACCEL_OK);
	}

	SECTION("invalid arguments")
	{
		ret = vaccel_torch_buffer_delete(nullptr);
		REQUIRE(ret == VACCEL_EINVAL);
	}
}

TEST_CASE("torch_buffer_take_data", "[ops][torch]")
{
	int ret;
	char data_str[] = "test";
	size_t const size = strlen(data_str) + 1;
	struct vaccel_torch_buffer *buffer = nullptr;
	void *buf_data;
	size_t buf_size;

	SECTION("success")
	{
		char *data = strdup(data_str);

		REQUIRE(vaccel_torch_buffer_new(&buffer, data, size) ==
			VACCEL_OK);

		ret = vaccel_torch_buffer_take_data(buffer, &buf_data,
						    &buf_size);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(buffer->data == nullptr);
		REQUIRE(buffer->size == 0);

		REQUIRE(buf_data == data);
		REQUIRE(buf_size == size);

		// ensure data is not freed on release
		REQUIRE(vaccel_torch_buffer_delete(buffer) == VACCEL_OK);
		free(data);
	}

	SECTION("invalid arguments")
	{
		ret = vaccel_torch_buffer_take_data(nullptr, &buf_data,
						    &buf_size);
		REQUIRE(ret == VACCEL_EINVAL);

		ret = vaccel_torch_buffer_take_data(buffer, nullptr, &buf_size);
		REQUIRE(ret == VACCEL_EINVAL);

		ret = vaccel_torch_buffer_take_data(buffer, &buf_data, nullptr);
		REQUIRE(ret == VACCEL_EINVAL);
	}
}

TEST_CASE("torch_tensor_init", "[ops][torch]")
{
	int ret;
	int64_t dims[] = { 1, 30 };
	struct vaccel_torch_tensor tensor;

	SECTION("success")
	{
		ret = vaccel_torch_tensor_init(&tensor, 2, dims,
					       VACCEL_TORCH_FLOAT);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(tensor.data_type == VACCEL_TORCH_FLOAT);
		REQUIRE(tensor.nr_dims == 2);
		REQUIRE(tensor.dims[0] == 1);
		REQUIRE(tensor.dims[1] == 30);
		REQUIRE(tensor.data == nullptr);
		REQUIRE(tensor.size == 0);
		REQUIRE_FALSE(tensor.owned);
		REQUIRE_FALSE(tensor.is_res);
		REQUIRE(tensor.res == nullptr);

		REQUIRE(vaccel_torch_tensor_release(&tensor) == VACCEL_OK);
	}

	SECTION("invalid arguments")
	{
		ret = vaccel_torch_tensor_init(nullptr, 2, dims,
					       VACCEL_TORCH_FLOAT);
		REQUIRE(ret == VACCEL_EINVAL);

		ret = vaccel_torch_tensor_init(&tensor, 0, dims,
					       VACCEL_TORCH_FLOAT);
		REQUIRE(ret == VACCEL_EINVAL);
	}

	SECTION("from resource")
	{
		struct vaccel_resource res;
		struct vaccel_session sess;
		REQUIRE(vaccel_session_init(&sess, 0) == VACCEL_OK);
		SECTION("Invalid arguments")
		{
			char *path =
				abs_path(SOURCE_ROOT,
					 "examples/models/torch/cnn_trace.pt");

			ret = vaccel_torch_tensor_init_from_res(
				&tensor, nullptr, 2, dims, VACCEL_TORCH_FLOAT);
			REQUIRE(ret == VACCEL_EINVAL);

			ret = vaccel_resource_init(&res, path,
						   VACCEL_RESOURCE_DATA);
			REQUIRE(ret == VACCEL_OK);

			/* Null blobs */
			ret = vaccel_torch_tensor_init_from_res(
				&tensor, &res, 2, dims, VACCEL_TORCH_FLOAT);
			REQUIRE(ret == VACCEL_EINVAL);

			REQUIRE(vaccel_resource_register(&res, &sess) ==
				VACCEL_OK);

			/* Wrong data type (file) */
			ret = vaccel_torch_tensor_init_from_res(
				&tensor, &res, 2, dims, VACCEL_TORCH_FLOAT);
			REQUIRE(ret == VACCEL_EINVAL);

			ret = vaccel_resource_unregister(&res, &sess);
			REQUIRE(ret == VACCEL_OK);

			ret = vaccel_resource_release(&res);
			REQUIRE(ret == VACCEL_OK);

			free(path);
		}

		SECTION("Success")
		{
			size_t size;
			int test_data[] = { 1, 2, 3, 4, 5 };

			ret = vaccel_resource_init_from_buf(
				&res, test_data, sizeof(test_data),
				VACCEL_RESOURCE_DATA, nullptr, true);
			REQUIRE(ret == VACCEL_OK);

			REQUIRE(vaccel_resource_register(&res, &sess) ==
				VACCEL_OK);

			ret = vaccel_torch_tensor_init_from_res(
				&tensor, &res, 2, dims, VACCEL_TORCH_FLOAT);
			REQUIRE(ret == VACCEL_OK);
			REQUIRE(tensor.is_res);
			REQUIRE(tensor.res == &res);

			ret = vaccel_torch_tensor_set_data(&tensor, test_data,
							   sizeof(test_data));
			REQUIRE(ret == VACCEL_EINVAL);

			ret = vaccel_torch_tensor_take_data(
				&tensor, (void **)&test_data, &size);
			REQUIRE(ret == VACCEL_EINVAL);

			ret = vaccel_torch_tensor_release(&tensor);
			REQUIRE(ret == VACCEL_OK);
			REQUIRE_FALSE(tensor.is_res);
			REQUIRE(tensor.res == nullptr);

			ret = vaccel_resource_unregister(&res, &sess);
			REQUIRE(ret == VACCEL_OK);

			ret = vaccel_resource_release(&res);
			REQUIRE(ret == VACCEL_OK);
		}

		ret = vaccel_session_release(&sess);
		REQUIRE(ret == VACCEL_OK);
	}
}

TEST_CASE("torch_tensor_release", "[ops][torch]")
{
	int ret;
	int64_t dims[] = { 1, 30 };
	struct vaccel_torch_tensor tensor;

	SECTION("success")
	{
		REQUIRE(vaccel_torch_tensor_init(&tensor, 2, dims,
						 VACCEL_TORCH_FLOAT) ==
			VACCEL_OK);
		// dummy values
		tensor.size = 1;
		tensor.owned = true;

		ret = vaccel_torch_tensor_release(&tensor);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(tensor.nr_dims == 0);
		REQUIRE(tensor.dims == nullptr);
		REQUIRE(tensor.data == nullptr);
		REQUIRE(tensor.size == 0);
		REQUIRE_FALSE(tensor.owned);
		REQUIRE_FALSE(tensor.is_res);
		REQUIRE(tensor.res == nullptr);
	}

	SECTION("invalid arguments")
	{
		ret = vaccel_torch_tensor_release(nullptr);
		REQUIRE(ret == VACCEL_EINVAL);
	}
}

TEST_CASE("torch_tensor_new", "[ops][torch]")
{
	int ret;
	int64_t dims[] = { 1, 30 };
	struct vaccel_torch_tensor *tensor = nullptr;

	SECTION("success")
	{
		ret = vaccel_torch_tensor_new(&tensor, 2, dims,
					      VACCEL_TORCH_FLOAT);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(tensor != nullptr);

		REQUIRE(vaccel_torch_tensor_delete(tensor) == VACCEL_OK);
	}

	SECTION("invalid arguments")
	{
		ret = vaccel_torch_tensor_new(nullptr, 2, dims,
					      VACCEL_TORCH_FLOAT);
		REQUIRE(ret == VACCEL_EINVAL);

		ret = vaccel_torch_tensor_new(&tensor, 0, dims,
					      VACCEL_TORCH_FLOAT);
		REQUIRE(ret == VACCEL_EINVAL);
	}

	SECTION("from resource")
	{
		struct vaccel_resource res;
		struct vaccel_session sess;
		REQUIRE(vaccel_session_init(&sess, 0) == VACCEL_OK);
		SECTION("Invalid arguments")
		{
			char *path =
				abs_path(SOURCE_ROOT,
					 "examples/models/torch/cnn_trace.pt");

			ret = vaccel_torch_tensor_new_from_res(
				&tensor, nullptr, 2, dims, VACCEL_TORCH_FLOAT);
			REQUIRE(ret == VACCEL_EINVAL);

			ret = vaccel_resource_init(&res, path,
						   VACCEL_RESOURCE_DATA);
			REQUIRE(ret == VACCEL_OK);

			/* Null blobs */
			ret = vaccel_torch_tensor_new_from_res(
				&tensor, &res, 2, dims, VACCEL_TORCH_FLOAT);
			REQUIRE(ret == VACCEL_EINVAL);

			REQUIRE(vaccel_resource_register(&res, &sess) ==
				VACCEL_OK);

			/* Wrong data type (file) */
			ret = vaccel_torch_tensor_new_from_res(
				&tensor, &res, 2, dims, VACCEL_TORCH_FLOAT);
			REQUIRE(ret == VACCEL_EINVAL);

			ret = vaccel_resource_unregister(&res, &sess);
			REQUIRE(ret == VACCEL_OK);

			ret = vaccel_resource_release(&res);
			REQUIRE(ret == VACCEL_OK);

			free(path);
		}

		SECTION("Success")
		{
			size_t size;
			int test_data[] = { 1, 2, 3, 4, 5 };

			ret = vaccel_resource_init_from_buf(
				&res, test_data, sizeof(test_data),
				VACCEL_RESOURCE_DATA, nullptr, true);
			REQUIRE(ret == VACCEL_OK);

			REQUIRE(vaccel_resource_register(&res, &sess) ==
				VACCEL_OK);

			ret = vaccel_torch_tensor_new_from_res(
				&tensor, &res, 2, dims, VACCEL_TORCH_FLOAT);
			REQUIRE(ret == VACCEL_OK);
			REQUIRE(tensor->is_res);
			REQUIRE(tensor->res == &res);

			ret = vaccel_torch_tensor_set_data(tensor, test_data,
							   sizeof(test_data));
			REQUIRE(ret == VACCEL_EINVAL);

			ret = vaccel_torch_tensor_take_data(
				tensor, (void **)&test_data, &size);
			REQUIRE(ret == VACCEL_EINVAL);

			ret = vaccel_torch_tensor_delete(tensor);
			REQUIRE(ret == VACCEL_OK);

			ret = vaccel_resource_unregister(&res, &sess);
			REQUIRE(ret == VACCEL_OK);

			ret = vaccel_resource_release(&res);
			REQUIRE(ret == VACCEL_OK);
		}

		ret = vaccel_session_release(&sess);
		REQUIRE(ret == VACCEL_OK);
	}
}

TEST_CASE("torch_tensor_allocate", "[ops][torch]")
{
	int ret;
	int64_t dims[] = { 1, 30 };
	struct vaccel_torch_tensor *tensor = nullptr;

	SECTION("success")
	{
		ret = vaccel_torch_tensor_allocate(&tensor, 2, dims,
						   VACCEL_TORCH_FLOAT, 1);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(tensor != nullptr);
		REQUIRE(tensor->data);
		REQUIRE(tensor->size == 1);
		REQUIRE(tensor->owned);
		REQUIRE_FALSE(tensor->is_res);
		REQUIRE(tensor->res == nullptr);

		REQUIRE(vaccel_torch_tensor_delete(tensor) == VACCEL_OK);
	}

	SECTION("success without data allocation")
	{
		ret = vaccel_torch_tensor_allocate(&tensor, 2, dims,
						   VACCEL_TORCH_FLOAT, 0);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(tensor != nullptr);
		REQUIRE(tensor->data == nullptr);
		REQUIRE(tensor->size == 0);
		REQUIRE_FALSE(tensor->owned);
		REQUIRE_FALSE(tensor->is_res);
		REQUIRE(tensor->res == nullptr);

		REQUIRE(vaccel_torch_tensor_delete(tensor) == VACCEL_OK);
	}

	SECTION("invalid arguments")
	{
		ret = vaccel_torch_tensor_allocate(nullptr, 2, dims,
						   VACCEL_TORCH_FLOAT, 1);
		REQUIRE(ret == VACCEL_EINVAL);

		ret = vaccel_torch_tensor_allocate(&tensor, 0, dims,
						   VACCEL_TORCH_FLOAT, 1);
		REQUIRE(ret == VACCEL_EINVAL);
	}
}

TEST_CASE("torch_tensor_delete", "[ops][torch]")
{
	int ret;
	int64_t dims[] = { 1, 30 };
	struct vaccel_torch_tensor *tensor = nullptr;

	SECTION("success")
	{
		REQUIRE(vaccel_torch_tensor_new(&tensor, 2, dims,
						VACCEL_TORCH_FLOAT) ==
			VACCEL_OK);

		ret = vaccel_torch_tensor_delete(tensor);
		REQUIRE(ret == VACCEL_OK);
	}

	SECTION("invalid arguments")
	{
		ret = vaccel_torch_tensor_delete(nullptr);
		REQUIRE(ret == VACCEL_EINVAL);
	}
}

TEST_CASE("torch_tensor_set_data", "[ops][torch]")
{
	int ret;
	int64_t dims[] = { 1, 30 };
	struct vaccel_torch_tensor tensor;

	REQUIRE(vaccel_torch_tensor_init(&tensor, 2, dims,
					 VACCEL_TORCH_FLOAT) == VACCEL_OK);

	SECTION("success")
	{
		ret = vaccel_torch_tensor_set_data(&tensor, dims, sizeof(dims));
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(tensor.data == dims);
		REQUIRE(tensor.size == sizeof(dims));
		REQUIRE_FALSE(tensor.owned);
		REQUIRE_FALSE(tensor.is_res);
		REQUIRE(tensor.res == nullptr);

		// ensure data is not freed on release
		REQUIRE(vaccel_torch_tensor_release(&tensor) == VACCEL_OK);
	}

	SECTION("invalid arguments")
	{
		ret = vaccel_torch_tensor_set_data(nullptr, dims, sizeof(dims));
		REQUIRE(ret == VACCEL_EINVAL);
	}

	REQUIRE(vaccel_torch_tensor_release(&tensor) == VACCEL_OK);
}

TEST_CASE("torch_tensor_take_data", "[ops][torch]")
{
	int ret;
	int64_t dims[] = { 1, 30 };
	struct vaccel_torch_tensor *tensor = nullptr;
	void *data;
	size_t size;

	REQUIRE(vaccel_torch_tensor_allocate(
			&tensor, 2, dims, VACCEL_TORCH_FLOAT, 1) == VACCEL_OK);

	SECTION("success")
	{
		ret = vaccel_torch_tensor_take_data(tensor, &data, &size);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(tensor->data == nullptr);
		REQUIRE(tensor->size == 0);
		REQUIRE_FALSE(tensor->owned);
		REQUIRE_FALSE(tensor->is_res);
		REQUIRE(tensor->res == nullptr);

		REQUIRE(data);
		REQUIRE(size == 1);

		free(data);
		// ensure data is not freed on release
		REQUIRE(vaccel_torch_tensor_release(tensor) == VACCEL_OK);
	}

	SECTION("invalid arguments")
	{
		ret = vaccel_torch_tensor_take_data(nullptr, &data, &size);
		REQUIRE(ret == VACCEL_EINVAL);

		ret = vaccel_torch_tensor_take_data(tensor, nullptr, &size);
		REQUIRE(ret == VACCEL_EINVAL);

		ret = vaccel_torch_tensor_take_data(tensor, &data, nullptr);
		REQUIRE(ret == VACCEL_EINVAL);
	}

	REQUIRE(vaccel_torch_tensor_delete(tensor) == VACCEL_OK);
}

TEST_CASE("torch_tensor_ops", "[ops][torch]")
{
	int64_t dims[] = { 1, 30 };

	// allocate data with dummy size
	struct vaccel_torch_tensor *tensor = nullptr;
	int ret = vaccel_torch_tensor_allocate(&tensor, 2, dims,
					       VACCEL_TORCH_FLOAT, 8);
	REQUIRE(ret == VACCEL_OK);

	// take data
	void *data;
	size_t size;
	ret = vaccel_torch_tensor_take_data(tensor, &data, &size);
	REQUIRE(ret == VACCEL_OK);

	// re-set other dummy data
	ret = vaccel_torch_tensor_set_data(tensor, dims, sizeof(dims));
	REQUIRE(ret == VACCEL_OK);

	ret = vaccel_torch_tensor_delete(tensor);
	REQUIRE(ret == VACCEL_OK);

	free(data);
}

TEST_CASE("torch_model_load", "[ops][torch]")
{
	int ret;
	struct vaccel_session sess;
	struct vaccel_resource model;
	char *model_path =
		abs_path(SOURCE_ROOT, "examples/models/torch/cnn_trace.pt");

	REQUIRE(vaccel_session_init(&sess, 0) == VACCEL_OK);
	REQUIRE(vaccel_resource_init(&model, model_path,
				     VACCEL_RESOURCE_DATA) == VACCEL_OK);

	SECTION("invalid arguments")
	{
		/* Null arguments*/
		ret = vaccel_torch_model_load(&sess, nullptr);
		REQUIRE(ret == VACCEL_EINVAL);
		ret = vaccel_torch_model_load(nullptr, &model);
		REQUIRE(ret == VACCEL_EINVAL);

		/* Invalid resource type*/
		ret = vaccel_torch_model_load(&sess, &model);
		REQUIRE(ret == VACCEL_EINVAL);
		model.type = VACCEL_RESOURCE_MODEL;

		/* Resource not registered */
		ret = vaccel_torch_model_load(&sess, &model);
		REQUIRE(ret == VACCEL_EPERM);
	}

	REQUIRE(vaccel_resource_register(&model, &sess) == VACCEL_OK);

	ret = vaccel_torch_model_load(&sess, &model);
	REQUIRE(ret == VACCEL_OK);

	REQUIRE(vaccel_resource_unregister(&model, &sess) == VACCEL_OK);
	REQUIRE(vaccel_session_release(&sess) == VACCEL_OK);
	REQUIRE(vaccel_resource_release(&model) == VACCEL_OK);

	free(model_path);
}

TEST_CASE("torch_inference", "[ops][torch]")
{
	struct vaccel_session sess;
	struct vaccel_resource model;
	int ret;
	char *model_path =
		abs_path(SOURCE_ROOT, "examples/models/torch/cnn_trace.pt");

	REQUIRE(vaccel_resource_init(&model, model_path,
				     VACCEL_RESOURCE_MODEL) == VACCEL_OK);
	REQUIRE(vaccel_session_init(&sess, 0) == VACCEL_OK);
	REQUIRE(vaccel_resource_register(&model, &sess) == VACCEL_OK);

	ret = vaccel_torch_model_load(&sess, &model);
	REQUIRE(ret == VACCEL_OK);

	struct vaccel_torch_buffer run_options;
	REQUIRE(vaccel_torch_buffer_init(&run_options, nullptr, 0) ==
		VACCEL_OK);

	int64_t dims[] = { 1, 30 };
	float data[30];
	for (float &i : data)
		i = 1.00;

	struct vaccel_torch_tensor *in = nullptr;
	REQUIRE(vaccel_torch_tensor_new(&in, 2, dims, VACCEL_TORCH_FLOAT) ==
		VACCEL_OK);
	REQUIRE(vaccel_torch_tensor_set_data(in, data, sizeof(data)) ==
		VACCEL_OK);

	struct vaccel_torch_tensor *out = nullptr;

	ret = vaccel_torch_model_run(&sess, &model, &run_options, &in, 1, &out,
				     1);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(out->data_type == in->data_type);
	REQUIRE(out->nr_dims == in->nr_dims);
	REQUIRE(out->dims[0] == in->dims[0]);
	REQUIRE(out->dims[1] == in->dims[1]);
	REQUIRE(out->size == in->size);
	for (size_t i = 0; i < out->size / sizeof(float); i++)
		REQUIRE(((float *)out->data)[i] == ((float *)in->data)[i]);
	REQUIRE(out->owned);

	REQUIRE(vaccel_torch_buffer_release(&run_options) == VACCEL_OK);
	REQUIRE(vaccel_torch_tensor_delete(in) == VACCEL_OK);
	REQUIRE(vaccel_torch_tensor_delete(out) == VACCEL_OK);

	REQUIRE(vaccel_resource_unregister(&model, &sess) == VACCEL_OK);
	REQUIRE(vaccel_session_release(&sess) == VACCEL_OK);
	REQUIRE(vaccel_resource_release(&model) == VACCEL_OK);

	free(model_path);
}
