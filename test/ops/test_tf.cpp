// SPDX-License-Identifier: Apache-2.0

/*
 * The code below performs unit testing for TensorFlow operations.
 *
 */

#include "utils.hpp"
#include "vaccel.h"
#include <catch.hpp>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

TEST_CASE("tf_buffer_init", "[ops][tf]")
{
	int ret;
	char data_str[] = "test";
	size_t const size = strlen(data_str) + 1;
	struct vaccel_tf_buffer buffer;

	SECTION("success")
	{
		char *data = strdup(data_str);

		ret = vaccel_tf_buffer_init(&buffer, data, size);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(buffer.data == data);
		REQUIRE(buffer.size == size);

		REQUIRE(vaccel_tf_buffer_release(&buffer) == VACCEL_OK);
	}

	SECTION("invalid arguments")
	{
		ret = vaccel_tf_buffer_init(nullptr, data_str, size);
		REQUIRE(ret == VACCEL_EINVAL);
	}
}

TEST_CASE("tf_buffer_release", "[ops][tf]")
{
	int ret;
	char data_str[] = "test";
	size_t const size = strlen(data_str) + 1;
	struct vaccel_tf_buffer buffer;

	SECTION("success")
	{
		char *data = strdup(data_str);

		REQUIRE(vaccel_tf_buffer_init(&buffer, data, size) ==
			VACCEL_OK);

		ret = vaccel_tf_buffer_release(&buffer);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(buffer.data == nullptr);
		REQUIRE(buffer.size == 0);
	}

	SECTION("invalid arguments")
	{
		ret = vaccel_tf_buffer_release(nullptr);
		REQUIRE(ret == VACCEL_EINVAL);
	}
}

TEST_CASE("tf_buffer_new", "[ops][tf]")
{
	int ret;
	char data_str[] = "test";
	size_t const size = strlen(data_str) + 1;
	struct vaccel_tf_buffer *buffer = nullptr;

	SECTION("success")
	{
		char *data = strdup(data_str);

		ret = vaccel_tf_buffer_new(&buffer, data, size);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(buffer != nullptr);

		REQUIRE(vaccel_tf_buffer_delete(buffer) == VACCEL_OK);
	}

	SECTION("invalid arguments")
	{
		ret = vaccel_tf_buffer_new(nullptr, data_str, size);
		REQUIRE(ret == VACCEL_EINVAL);
	}
}

TEST_CASE("tf_buffer_delete", "[ops][tf]")
{
	int ret;
	char data_str[] = "test";
	size_t const size = strlen(data_str) + 1;
	struct vaccel_tf_buffer *buffer = nullptr;

	SECTION("success")
	{
		char *data = strdup(data_str);

		REQUIRE(vaccel_tf_buffer_new(&buffer, data, size) == VACCEL_OK);

		ret = vaccel_tf_buffer_delete(buffer);
		REQUIRE(ret == VACCEL_OK);
	}

	SECTION("invalid arguments")
	{
		ret = vaccel_tf_buffer_delete(nullptr);
		REQUIRE(ret == VACCEL_EINVAL);
	}
}

TEST_CASE("tf_buffer_take_data", "[ops][tf]")
{
	int ret;
	char data_str[] = "test";
	size_t const size = strlen(data_str) + 1;
	struct vaccel_tf_buffer *buffer = nullptr;
	void *buf_data;
	size_t buf_size;

	SECTION("success")
	{
		char *data = strdup(data_str);

		REQUIRE(vaccel_tf_buffer_new(&buffer, data, size) == VACCEL_OK);

		ret = vaccel_tf_buffer_take_data(buffer, &buf_data, &buf_size);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(buffer->data == nullptr);
		REQUIRE(buffer->size == 0);

		REQUIRE(buf_data == data);
		REQUIRE(buf_size == size);

		// ensure data is not freed on release
		REQUIRE(vaccel_tf_buffer_delete(buffer) == VACCEL_OK);
		free(data);
	}

	SECTION("invalid arguments")
	{
		ret = vaccel_tf_buffer_take_data(nullptr, &buf_data, &buf_size);
		REQUIRE(ret == VACCEL_EINVAL);

		ret = vaccel_tf_buffer_take_data(buffer, nullptr, &buf_size);
		REQUIRE(ret == VACCEL_EINVAL);

		ret = vaccel_tf_buffer_take_data(buffer, &buf_data, nullptr);
		REQUIRE(ret == VACCEL_EINVAL);
	}
}

TEST_CASE("tf_node_init", "[ops][tf]")
{
	int ret;
	const char *name = "test";
	int const id = 1;
	struct vaccel_tf_node node;

	SECTION("success")
	{
		ret = vaccel_tf_node_init(&node, name, id);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(strcmp(node.name, name) == 0);
		REQUIRE(node.id == id);

		REQUIRE(vaccel_tf_node_release(&node) == VACCEL_OK);
	}

	SECTION("invalid arguments")
	{
		ret = vaccel_tf_node_init(nullptr, name, id);
		REQUIRE(ret == VACCEL_EINVAL);

		ret = vaccel_tf_node_init(&node, nullptr, id);
		REQUIRE(ret == VACCEL_EINVAL);
	}
}

TEST_CASE("tf_node_release", "[ops][tf]")
{
	int ret;
	const char *name = "test";
	int const id = 1;
	struct vaccel_tf_node node;

	SECTION("success")
	{
		REQUIRE(vaccel_tf_node_init(&node, name, id) == VACCEL_OK);

		ret = vaccel_tf_node_release(&node);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(node.name == nullptr);
		REQUIRE(node.id == -1);
	}

	SECTION("invalid arguments")
	{
		ret = vaccel_tf_node_release(nullptr);
		REQUIRE(ret == VACCEL_EINVAL);
	}
}

TEST_CASE("tf_node_new", "[ops][tf]")
{
	int ret;
	const char *name = "test";
	int const id = 1;
	struct vaccel_tf_node *node = nullptr;

	SECTION("success")
	{
		ret = vaccel_tf_node_new(&node, name, id);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(node != nullptr);

		REQUIRE(vaccel_tf_node_delete(node) == VACCEL_OK);
	}

	SECTION("invalid arguments")
	{
		ret = vaccel_tf_node_new(nullptr, name, id);
		REQUIRE(ret == VACCEL_EINVAL);

		ret = vaccel_tf_node_new(&node, nullptr, id);
		REQUIRE(ret == VACCEL_EINVAL);
	}
}

TEST_CASE("tf_node_delete", "[ops][tf]")
{
	int ret;
	const char *name = "test";
	int const id = 1;
	struct vaccel_tf_node *node = nullptr;

	SECTION("success")
	{
		REQUIRE(vaccel_tf_node_new(&node, name, id) == VACCEL_OK);

		ret = vaccel_tf_node_delete(node);
		REQUIRE(ret == VACCEL_OK);
	}

	SECTION("invalid arguments")
	{
		ret = vaccel_tf_node_delete(nullptr);
		REQUIRE(ret == VACCEL_EINVAL);
	}
}

TEST_CASE("tf_status_init", "[ops][tf]")
{
	int ret;
	uint8_t const error_code = 1;
	const char *message = "test";
	struct vaccel_tf_status status;

	SECTION("success")
	{
		ret = vaccel_tf_status_init(&status, error_code, message);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(status.error_code == error_code);
		REQUIRE(strcmp(status.message, message) == 0);

		REQUIRE(vaccel_tf_status_release(&status) == VACCEL_OK);
	}

	SECTION("invalid arguments")
	{
		ret = vaccel_tf_status_init(nullptr, error_code, message);
		REQUIRE(ret == VACCEL_EINVAL);

		ret = vaccel_tf_status_init(&status, error_code, nullptr);
		REQUIRE(ret == VACCEL_EINVAL);
	}
}

TEST_CASE("tf_status_release", "[ops][tf]")
{
	int ret;
	uint8_t const error_code = 1;
	const char *message = "test";
	struct vaccel_tf_status status;

	SECTION("success")
	{
		REQUIRE(vaccel_tf_status_init(&status, error_code, message) ==
			VACCEL_OK);

		ret = vaccel_tf_status_release(&status);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(status.error_code == 0);
		REQUIRE(status.message == nullptr);
	}

	SECTION("invalid arguments")
	{
		ret = vaccel_tf_status_release(nullptr);
		REQUIRE(ret == VACCEL_EINVAL);
	}
}

TEST_CASE("tf_status_new", "[ops][tf]")
{
	int ret;
	uint8_t const error_code = 1;
	const char *message = "test";
	struct vaccel_tf_status *status = nullptr;

	SECTION("success")
	{
		ret = vaccel_tf_status_new(&status, error_code, message);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(status != nullptr);

		REQUIRE(vaccel_tf_status_delete(status) == VACCEL_OK);
	}

	SECTION("invalid arguments")
	{
		ret = vaccel_tf_status_new(nullptr, error_code, message);
		REQUIRE(ret == VACCEL_EINVAL);

		ret = vaccel_tf_status_new(&status, error_code, nullptr);
		REQUIRE(ret == VACCEL_EINVAL);
	}
}

TEST_CASE("tf_status_delete", "[ops][tf]")
{
	int ret;
	uint8_t const error_code = 1;
	const char *message = "test";
	struct vaccel_tf_status *status = nullptr;

	SECTION("success")
	{
		REQUIRE(vaccel_tf_status_new(&status, error_code, message) ==
			VACCEL_OK);

		ret = vaccel_tf_status_delete(status);
		REQUIRE(ret == VACCEL_OK);
	}

	SECTION("invalid arguments")
	{
		ret = vaccel_tf_status_delete(nullptr);
		REQUIRE(ret == VACCEL_EINVAL);
	}
}

TEST_CASE("tf_tensor_init", "[ops][tf]")
{
	int ret;
	int64_t dims[] = { 1, 30 };
	struct vaccel_tf_tensor tensor;

	SECTION("success")
	{
		ret = vaccel_tf_tensor_init(&tensor, 2, dims, VACCEL_TF_FLOAT);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(tensor.data_type == VACCEL_TF_FLOAT);
		REQUIRE(tensor.nr_dims == 2);
		REQUIRE(tensor.dims[0] == 1);
		REQUIRE(tensor.dims[1] == 30);
		REQUIRE(tensor.data == nullptr);
		REQUIRE(tensor.size == 0);
		REQUIRE_FALSE(tensor.owned);

		REQUIRE(vaccel_tf_tensor_release(&tensor) == VACCEL_OK);
	}

	SECTION("invalid arguments")
	{
		ret = vaccel_tf_tensor_init(nullptr, 2, dims, VACCEL_TF_FLOAT);
		REQUIRE(ret == VACCEL_EINVAL);

		ret = vaccel_tf_tensor_init(&tensor, 0, dims, VACCEL_TF_FLOAT);
		REQUIRE(ret == VACCEL_EINVAL);
	}
}

TEST_CASE("tf_tensor_release", "[ops][tf]")
{
	int ret;
	int64_t dims[] = { 1, 30 };
	struct vaccel_tf_tensor tensor;

	SECTION("success")
	{
		REQUIRE(vaccel_tf_tensor_init(&tensor, 2, dims,
					      VACCEL_TF_FLOAT) == VACCEL_OK);
		// dummy values
		tensor.size = 1;
		tensor.owned = true;

		ret = vaccel_tf_tensor_release(&tensor);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(tensor.nr_dims == 0);
		REQUIRE(tensor.dims == nullptr);
		REQUIRE(tensor.data == nullptr);
		REQUIRE(tensor.size == 0);
		REQUIRE_FALSE(tensor.owned);
	}

	SECTION("invalid arguments")
	{
		ret = vaccel_tf_tensor_release(nullptr);
		REQUIRE(ret == VACCEL_EINVAL);
	}
}

TEST_CASE("tf_tensor_new", "[ops][tf]")
{
	int ret;
	int64_t dims[] = { 1, 30 };
	struct vaccel_tf_tensor *tensor = nullptr;

	SECTION("success")
	{
		ret = vaccel_tf_tensor_new(&tensor, 2, dims, VACCEL_TF_FLOAT);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(tensor != nullptr);

		REQUIRE(vaccel_tf_tensor_delete(tensor) == VACCEL_OK);
	}

	SECTION("invalid arguments")
	{
		ret = vaccel_tf_tensor_new(nullptr, 2, dims, VACCEL_TF_FLOAT);
		REQUIRE(ret == VACCEL_EINVAL);

		ret = vaccel_tf_tensor_new(&tensor, 0, dims, VACCEL_TF_FLOAT);
		REQUIRE(ret == VACCEL_EINVAL);
	}
}

TEST_CASE("tf_tensor_allocate", "[ops][tf]")
{
	int ret;
	int64_t dims[] = { 1, 30 };
	struct vaccel_tf_tensor *tensor = nullptr;

	SECTION("success")
	{
		ret = vaccel_tf_tensor_allocate(&tensor, 2, dims,
						VACCEL_TF_FLOAT, 1);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(tensor != nullptr);
		REQUIRE(tensor->data);
		REQUIRE(tensor->size == 1);
		REQUIRE(tensor->owned);

		REQUIRE(vaccel_tf_tensor_delete(tensor) == VACCEL_OK);
	}

	SECTION("success without data allocation")
	{
		ret = vaccel_tf_tensor_allocate(&tensor, 2, dims,
						VACCEL_TF_FLOAT, 0);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(tensor != nullptr);
		REQUIRE(tensor->data == nullptr);
		REQUIRE(tensor->size == 0);
		REQUIRE_FALSE(tensor->owned);

		REQUIRE(vaccel_tf_tensor_delete(tensor) == VACCEL_OK);
	}

	SECTION("invalid arguments")
	{
		ret = vaccel_tf_tensor_allocate(nullptr, 2, dims,
						VACCEL_TF_FLOAT, 1);
		REQUIRE(ret == VACCEL_EINVAL);

		ret = vaccel_tf_tensor_allocate(&tensor, 0, dims,
						VACCEL_TF_FLOAT, 1);
		REQUIRE(ret == VACCEL_EINVAL);
	}
}

TEST_CASE("tf_tensor_delete", "[ops][tf]")
{
	int ret;
	int64_t dims[] = { 1, 30 };
	struct vaccel_tf_tensor *tensor = nullptr;

	SECTION("success")
	{
		REQUIRE(vaccel_tf_tensor_new(&tensor, 2, dims,
					     VACCEL_TF_FLOAT) == VACCEL_OK);

		ret = vaccel_tf_tensor_delete(tensor);
		REQUIRE(ret == VACCEL_OK);
	}

	SECTION("invalid arguments")
	{
		ret = vaccel_tf_tensor_delete(nullptr);
		REQUIRE(ret == VACCEL_EINVAL);
	}
}

TEST_CASE("tf_tensor_set_data", "[ops][tf]")
{
	int ret;
	int64_t dims[] = { 1, 30 };
	struct vaccel_tf_tensor tensor;

	REQUIRE(vaccel_tf_tensor_init(&tensor, 2, dims, VACCEL_TF_FLOAT) ==
		VACCEL_OK);

	SECTION("success")
	{
		ret = vaccel_tf_tensor_set_data(&tensor, dims, sizeof(dims));
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(tensor.data == dims);
		REQUIRE(tensor.size == sizeof(dims));
		REQUIRE_FALSE(tensor.owned);

		// ensure data is not freed on release
		REQUIRE(vaccel_tf_tensor_release(&tensor) == VACCEL_OK);
	}

	SECTION("invalid arguments")
	{
		ret = vaccel_tf_tensor_set_data(nullptr, dims, sizeof(dims));
		REQUIRE(ret == VACCEL_EINVAL);
	}

	REQUIRE(vaccel_tf_tensor_release(&tensor) == VACCEL_OK);
}

TEST_CASE("tf_tensor_take_data", "[ops][tf]")
{
	int ret;
	int64_t dims[] = { 1, 30 };
	struct vaccel_tf_tensor *tensor = nullptr;
	void *data;
	size_t size;

	REQUIRE(vaccel_tf_tensor_allocate(&tensor, 2, dims, VACCEL_TF_FLOAT,
					  1) == VACCEL_OK);

	SECTION("success")
	{
		ret = vaccel_tf_tensor_take_data(tensor, &data, &size);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(tensor->data == nullptr);
		REQUIRE(tensor->size == 0);
		REQUIRE_FALSE(tensor->owned);

		REQUIRE(data);
		REQUIRE(size == 1);

		free(data);
		// ensure data is not freed on release
		REQUIRE(vaccel_tf_tensor_release(tensor) == VACCEL_OK);
	}

	SECTION("invalid arguments")
	{
		ret = vaccel_tf_tensor_take_data(nullptr, &data, &size);
		REQUIRE(ret == VACCEL_EINVAL);

		ret = vaccel_tf_tensor_take_data(tensor, nullptr, &size);
		REQUIRE(ret == VACCEL_EINVAL);

		ret = vaccel_tf_tensor_take_data(tensor, &data, nullptr);
		REQUIRE(ret == VACCEL_EINVAL);
	}

	REQUIRE(vaccel_tf_tensor_delete(tensor) == VACCEL_OK);
}

TEST_CASE("tf_tensor_ops", "[ops][tf]")
{
	int64_t dims[] = { 1, 30 };

	// allocate data with dummy size
	struct vaccel_tf_tensor *tensor = nullptr;
	int ret =
		vaccel_tf_tensor_allocate(&tensor, 2, dims, VACCEL_TF_FLOAT, 8);
	REQUIRE(ret == VACCEL_OK);

	// take data
	void *data;
	size_t size;
	ret = vaccel_tf_tensor_take_data(tensor, &data, &size);
	REQUIRE(ret == VACCEL_OK);

	// re-set other dummy data
	ret = vaccel_tf_tensor_set_data(tensor, dims, sizeof(dims));
	REQUIRE(ret == VACCEL_OK);

	ret = vaccel_tf_tensor_delete(tensor);
	REQUIRE(ret == VACCEL_OK);

	free(data);
}

TEST_CASE("tf_inference", "[ops][tf]")
{
	struct vaccel_session vsess;
	struct vaccel_resource model;
	int ret;
	char *model_path = abs_path(SOURCE_ROOT, "examples/models/tf/lstm2");

	REQUIRE(vaccel_resource_init(&model, model_path,
				     VACCEL_RESOURCE_MODEL) == VACCEL_OK);
	REQUIRE(vaccel_session_init(&vsess, 0) == VACCEL_OK);
	REQUIRE(vaccel_resource_register(&model, &vsess) == VACCEL_OK);

	struct vaccel_tf_status status;

	ret = vaccel_tf_model_load(&vsess, &model, &status);
	REQUIRE(ret == VACCEL_OK);

	REQUIRE(vaccel_tf_status_release(&status) == VACCEL_OK);

	struct vaccel_tf_buffer run_options;
	REQUIRE(vaccel_tf_buffer_init(&run_options, nullptr, 0) == VACCEL_OK);

	const char *in_node_name = "serving_default_input_1";
	struct vaccel_tf_node in_node;
	REQUIRE(vaccel_tf_node_init(&in_node, in_node_name, 0) == VACCEL_OK);

	int64_t dims[] = { 1, 30 };
	float data[30];
	for (float &i : data)
		i = 1.00;

	struct vaccel_tf_tensor *in = nullptr;
	REQUIRE(vaccel_tf_tensor_new(&in, 2, dims, VACCEL_TF_FLOAT) ==
		VACCEL_OK);
	REQUIRE(vaccel_tf_tensor_set_data(in, data, sizeof(data)) == VACCEL_OK);

	const char *out_node_name = "StatefulPartitionedCall";
	struct vaccel_tf_node out_node;
	REQUIRE(vaccel_tf_node_init(&out_node, out_node_name, 0) == VACCEL_OK);

	struct vaccel_tf_tensor *out;

	ret = vaccel_tf_model_run(&vsess, &model, &run_options, &in_node, &in,
				  1, &out_node, &out, 1, &status);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(out->data_type == in->data_type);
	REQUIRE(out->nr_dims == in->nr_dims);
	REQUIRE(out->dims[0] == in->dims[0]);
	REQUIRE(out->dims[1] == in->dims[1]);
	REQUIRE(out->size == in->size);
	for (size_t i = 0; i < out->size / sizeof(float); i++)
		REQUIRE(((float *)out->data)[i] == ((float *)in->data)[i]);
	REQUIRE(out->owned);
	REQUIRE(status.error_code == 0);

	REQUIRE(vaccel_tf_status_release(&status) == VACCEL_OK);

	REQUIRE(vaccel_tf_buffer_release(&run_options) == VACCEL_OK);
	REQUIRE(vaccel_tf_node_release(&in_node) == VACCEL_OK);
	REQUIRE(vaccel_tf_node_release(&out_node) == VACCEL_OK);
	REQUIRE(vaccel_tf_tensor_delete(in) == VACCEL_OK);
	REQUIRE(vaccel_tf_tensor_delete(out) == VACCEL_OK);

	ret = vaccel_tf_model_unload(&vsess, &model, &status);
	REQUIRE(ret == VACCEL_OK);

	REQUIRE(vaccel_tf_status_release(&status) == VACCEL_OK);

	REQUIRE(vaccel_resource_unregister(&model, &vsess) == VACCEL_OK);
	REQUIRE(vaccel_session_release(&vsess) == VACCEL_OK);
	REQUIRE(vaccel_resource_release(&model) == VACCEL_OK);

	free(model_path);
}
