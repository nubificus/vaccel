// SPDX-License-Identifier: Apache-2.0

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

#include <ops/tflite.h>
#include <vaccel.h>

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
	printf("Created new model %lld\n", model.id);

	ret = vaccel_session_init(&vsess, 0);
	if (ret) {
		fprintf(stderr, "Could not initialize vAccel session\n");
		goto destroy_resource;
	}

	printf("Initialized vAccel session %u\n", vsess.session_id);

	ret = vaccel_resource_register(&model, &vsess);
	if (ret) {
		fprintf(stderr, "Could not register model with session\n");
		goto close_session;
	}

	uint8_t status;
	ret = vaccel_tflite_session_load(&vsess, &model);
	if (ret) {
		fprintf(stderr, "Could not load graph from model\n");
		goto unregister_resource;
	}

	/* Input tensors */
	int32_t dims[] = { 1, 30 };
	float data[30];
	for (int i = 0; i < 30; ++i)
		data[i] = 1.00;

	struct vaccel_tflite_tensor *in =
		vaccel_tflite_tensor_new(2, dims, VACCEL_TFLITE_FLOAT32);
	if (!in) {
		fprintf(stderr, "Could not allocate memory\n");
		exit(1);
	}

	in->data = data;
	in->size = sizeof(float) * 30;

	/* Output tensors */
	struct vaccel_tflite_tensor *out;

	ret = vaccel_tflite_session_run(&vsess, &model, &in, 1, &out, 1,
					&status);
	if (ret) {
		fprintf(stderr, "Could not run model: %d\n", ret);
		goto unload_session;
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

	if (vaccel_tflite_tensor_destroy(out))
		fprintf(stderr, "Could not destroy out tensor\n");
unload_session:
	if (vaccel_tflite_tensor_destroy(in))
		fprintf(stderr, "Could not destroy in tensor\n");

	if (vaccel_tflite_session_delete(&vsess, &model))
		fprintf(stderr, "Could not delete tf session\n");
unregister_resource:
	if (vaccel_resource_unregister(&model, &vsess))
		fprintf(stderr, "Could not unregister model with session\n");
close_session:
	if (vaccel_session_free(&vsess))
		fprintf(stderr, "Could not clear session\n");
destroy_resource:
	vaccel_resource_release(&model);

	return ret;
}
