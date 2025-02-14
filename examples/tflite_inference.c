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
		fprintf(stderr, "usage: %s model\n", argv[0]);
		exit(1);
	}

	char *model_path = argv[1];

	int ret =
		vaccel_resource_init(&model, model_path, VACCEL_RESOURCE_MODEL);
	if (ret) {
		vaccel_error("Could not create model resource");
		return ret;
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

	uint8_t status;
	ret = vaccel_tflite_session_load(&vsess, &model);
	if (ret) {
		fprintf(stderr, "Could not load session from model\n");
		goto unregister_resource;
	}

	/* Input tensors */
	int32_t dims[] = { 1, 30 };
	float data[30];
	for (int i = 0; i < 30; ++i)
		data[i] = 1.00;

	struct vaccel_tflite_tensor *in;
	ret = vaccel_tflite_tensor_new(&in, 2, dims, VACCEL_TFLITE_FLOAT32);
	if (ret) {
		fprintf(stderr, "Could not create input tensor\n");
		goto delete_tf_session;
	}

	ret = vaccel_tflite_tensor_set_data(in, data, sizeof(data));
	if (ret) {
		fprintf(stderr, "Could not set input tensor data\n");
		goto delete_in_tensor;
	}

	/* Output tensors */
	struct vaccel_tflite_tensor *out;

	ret = vaccel_tflite_session_run(&vsess, &model, &in, 1, &out, 1,
					&status);
	if (ret) {
		fprintf(stderr, "Could not run model: %d\n", ret);
		goto delete_in_tensor;
	}

	printf("Success!\n");
	printf("Output tensor => type:%u nr_dims:%u\n", out->data_type,
	       out->nr_dims);
	for (int i = 0; i < out->nr_dims; ++i)
		printf("dim[%d]: %" PRId32 "\n", i, out->dims[i]);

	printf("Result Tensor :\n");
	float *offsets = (float *)out->data;
	for (unsigned int i = 0; i < min(10, out->size / sizeof(float)); ++i)
		printf("%f\n", offsets[i]);

	if (vaccel_tflite_tensor_delete(out))
		fprintf(stderr, "Could not delete output tensor\n");
delete_in_tensor:
	if (vaccel_tflite_tensor_delete(in))
		fprintf(stderr, "Could not delete input tensor\n");
delete_tf_session:
	if (vaccel_tflite_session_delete(&vsess, &model))
		fprintf(stderr, "Could not delete tf session\n");
unregister_resource:
	if (vaccel_resource_unregister(&model, &vsess))
		fprintf(stderr, "Could not unregister model from session\n");
release_session:
	if (vaccel_session_release(&vsess))
		fprintf(stderr, "Could not release session\n");
release_resource:
	vaccel_resource_release(&model);

	return ret;
}
