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
	struct vaccel_single_model model;

	if (argc != 2) {
		fprintf(stderr, "usage: %s model\n", argv[0]);
		exit(1);
	}

	const char *model_path = argv[1];

	int ret = vaccel_single_model_set_path(&model, model_path);
	if (ret) {
		fprintf(stderr,
			"Could not set model path to TensorFlow Lite model\n");
		exit(1);
	}

	ret = vaccel_single_model_register(&model);
	if (ret) {
		fprintf(stderr,
			"Could not register TensorFlow Lite model resource\n");
		exit(1);
	}

	printf("Created new model %lld\n", vaccel_single_model_get_id(&model));

	ret = vaccel_sess_init(&vsess, 0);
	if (ret) {
		fprintf(stderr, "Could not initialize vAccel session\n");
		goto destroy_resource;
	}

	printf("Initialized vAccel session %u\n", vsess.session_id);

	ret = vaccel_sess_register(&vsess, model.resource);
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

unload_session:
	vaccel_tflite_session_delete(&vsess, &model);

unregister_resource:
	vaccel_sess_unregister(&vsess, model.resource);

close_session:
	vaccel_sess_free(&vsess);

destroy_resource:
	vaccel_single_model_destroy(&model);

	return ret;
}
