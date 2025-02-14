// SPDX-License-Identifier: Apache-2.0

#include "vaccel.h"
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define min(a, b) ((a) < (b) ? (a) : (b))

int main(int argc, char *argv[])
{
	struct vaccel_session vsess;
	struct vaccel_resource model;

	if (argc != 2) {
		fprintf(stderr, "Usage: %s model", argv[0]);
		exit(1);
	}

	char *model_path = argv[1];

	int ret =
		vaccel_resource_init(&model, model_path, VACCEL_RESOURCE_MODEL);
	if (ret) {
		fprintf(stderr, "Could not create model resource\n\n");
		exit(1);
	}

	printf("Created new model %" PRId64 "\n", model.id);

	ret = vaccel_session_init(&vsess, 0);
	if (ret) {
		fprintf(stderr, "Could not initialize vAccel session\n");
		goto release_resource;
	}

	printf("Initialized vAccel session %" PRId64 "\n", vsess.id);

	ret = vaccel_resource_register(&model, &vsess);
	if (ret) {
		fprintf(stderr, "Could not register model with session\n");
		goto release_session;
	}

	struct vaccel_tf_status status;
	status.message = NULL;

	ret = vaccel_tf_session_load(&vsess, &model, &status);
	if (ret) {
		fprintf(stderr, "Could not load graph from model\n");
		goto unregister_resource;
	}

	if (vaccel_tf_status_release(&status))
		fprintf(stderr, "Could not release session load status\n");

	struct vaccel_tf_buffer run_options = { NULL, 0 };

	/* Input tensors & nodes */
	struct vaccel_tf_node in_node = { "serving_default_input_1", 0 };

	int64_t dims[] = { 1, 30 };
	float data[30];
	for (int i = 0; i < 30; ++i)
		data[i] = 1.00;

	struct vaccel_tf_tensor *in;
	ret = vaccel_tf_tensor_new(&in, 2, dims, VACCEL_TF_FLOAT);
	if (ret) {
		fprintf(stderr, "Could not create input tensor\n");
		goto delete_session;
	}

	ret = vaccel_tf_tensor_set_data(in, data, sizeof(data));
	if (ret) {
		fprintf(stderr, "Could not set input tensor data\n");
		goto delete_in_tensor;
	}

	/* Output tensors & nodes */
	struct vaccel_tf_node out_node = { "StatefulPartitionedCall", 0 };
	struct vaccel_tf_tensor *out;

	ret = vaccel_tf_session_run(&vsess, &model, &run_options, &in_node, &in,
				    1, &out_node, &out, 1, &status);
	if (ret) {
		fprintf(stderr, "Could not run model: %d", ret);
		goto delete_in_tensor;
	}

	printf("Success!\n");
	printf("Output tensor => type:%u nr_dims:%d\n", out->data_type,
	       out->nr_dims);
	for (int i = 0; i < out->nr_dims; ++i)
		printf("dim[%d]: %" PRId64 "\n", i, out->dims[i]);

	printf("Result Tensor :\n");
	float *offsets = (float *)out->data;
	for (unsigned int i = 0; i < min(10, out->size / sizeof(float)); ++i)
		printf("%f\n", offsets[i]);

	if (vaccel_tf_status_release(&status))
		fprintf(stderr, "Could not release session run status\n");

	if (vaccel_tf_tensor_delete(out))
		fprintf(stderr, "Could not delete output tensor\n");
delete_in_tensor:
	if (vaccel_tf_tensor_delete(in))
		fprintf(stderr, "Could not delete input tensor\n");
delete_session:
	if (vaccel_tf_session_delete(&vsess, &model, &status))
		fprintf(stderr, "Could not delete tf session\n");
	if (vaccel_tf_status_release(&status))
		fprintf(stderr, "Could not release session delete status\n");
unregister_resource:
	if (vaccel_resource_unregister(&model, &vsess))
		fprintf(stderr, "Could not unregister model from session\n");
release_session:
	if (vaccel_session_release(&vsess))
		fprintf(stderr, "Could not release session\n");
release_resource:
	if (vaccel_resource_release(&model))
		fprintf(stderr, "Could not release resource\n");

	return ret;
}
