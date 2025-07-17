// SPDX-License-Identifier: Apache-2.0

#include "common/inference_helpers.h"
#include "vaccel.h"
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
	int ret;
	struct vaccel_session sess;
	struct vaccel_resource model;
	struct vaccel_prof_region tflite_model_load_stats =
		VACCEL_PROF_REGION_INIT("tflite_model_load");
	struct vaccel_prof_region tflite_model_run_stats =
		VACCEL_PROF_REGION_INIT("tflite_model_run");
	struct vaccel_prof_region tflite_model_unload_stats =
		VACCEL_PROF_REGION_INIT("tflite_model_unload");

	if (argc < 4 || argc > 5) {
		fprintf(stderr,
			"Usage: %s <image_file> <model_dir> <labels_file> [iterations]\n",
			argv[0]);
		return VACCEL_EINVAL;
	}

	ret = vaccel_resource_init(&model, argv[2], VACCEL_RESOURCE_MODEL);
	if (ret) {
		fprintf(stderr, "Could not initialize model resource\n");
		return ret;
	}
	printf("Initialized model resource %" PRId64 "\n", model.id);

	ret = vaccel_session_init(&sess, 0);
	if (ret) {
		fprintf(stderr, "Could not initialize vAccel session\n");
		goto release_resource;
	}

	printf("Initialized vAccel session %" PRId64 "\n", sess.id);

	ret = vaccel_resource_register(&model, &sess);
	if (ret) {
		fprintf(stderr, "Could not register model with session\n");
		goto release_session;
	}

	vaccel_prof_region_start(&tflite_model_load_stats);

	ret = vaccel_tflite_model_load(&sess, &model);

	vaccel_prof_region_stop(&tflite_model_load_stats);

	if (ret) {
		fprintf(stderr, "Could not load model\n");
		goto unregister_resource;
	}

	int32_t dims[] = { 1, INFERENCE_IMAGE_HEIGHT, INFERENCE_IMAGE_WIDTH,
			   INFERENCE_IMAGE_CHANNELS };

	struct vaccel_tflite_tensor *in;
	ret = vaccel_tflite_tensor_new(&in, 4, dims, VACCEL_TFLITE_FLOAT32);
	if (ret) {
		fprintf(stderr, "Could not create input tensor\n");
		goto delete_tflite_model;
	}

	ret = inference_load_and_preprocess_image(argv[1],
						  INFERENCE_IMAGE_FORMAT_TF,
						  (float **)&in->data,
						  &in->size);
	if (ret) {
		fprintf(stderr, "Could not load and preprocess image\n");
		goto delete_in_tensor;
	}
	in->owned = true;

	struct vaccel_tflite_tensor *out = NULL;
	uint8_t status;

	const int iter = (argc > 4) ? atoi(argv[4]) : 1;
	for (int i = 0; i < iter; i++) {
		vaccel_prof_region_start(&tflite_model_run_stats);

		ret = vaccel_tflite_model_run(&sess, &model, &in, 1, &out, 1,
					      &status);

		vaccel_prof_region_stop(&tflite_model_run_stats);

		printf("Session run status: %" PRIu8 "\n", status);
		if (ret) {
			fprintf(stderr, "Could not run model: %d\n", ret);
			goto delete_in_tensor;
		}

		printf("Success!\n");
		printf("Output tensor => type:%u nr_dims:%u size:%zuB\n",
		       out->data_type, out->nr_dims, out->size);

		ret = inference_process_result(out->data, argv[3]);
		if (ret)
			fprintf(stderr, "Could not process result\n");

		if (vaccel_tflite_tensor_delete(out)) {
			fprintf(stderr, "Could not delete output tensor\n");
			goto delete_in_tensor;
		}

		if (ret)
			goto delete_in_tensor;
	}

delete_in_tensor:
	if (vaccel_tflite_tensor_delete(in))
		fprintf(stderr, "Could not delete input tensor\n");
delete_tflite_model:
	vaccel_prof_region_start(&tflite_model_unload_stats);

	if (vaccel_tflite_model_unload(&sess, &model))
		fprintf(stderr, "Could not unload model\n");

	vaccel_prof_region_stop(&tflite_model_unload_stats);
unregister_resource:
	if (vaccel_resource_unregister(&model, &sess))
		fprintf(stderr, "Could not unregister model from session\n");
release_session:
	if (vaccel_session_release(&sess))
		fprintf(stderr, "Could not release session\n");
release_resource:
	vaccel_resource_release(&model);

	vaccel_prof_region_print(&tflite_model_load_stats);
	vaccel_prof_region_print(&tflite_model_unload_stats);
	vaccel_prof_region_print(&tflite_model_run_stats);

	vaccel_prof_region_release(&tflite_model_load_stats);
	vaccel_prof_region_release(&tflite_model_unload_stats);
	vaccel_prof_region_release(&tflite_model_run_stats);

	return ret;
}
