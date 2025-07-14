// SPDX-License-Identifier: Apache-2.0

#define _POSIX_C_SOURCE 200809L

#include "common/inference_helpers.h"
#include "vaccel.h"
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
	int ret;
	struct vaccel_session sess;
	struct vaccel_resource model;
	int64_t dims[] = { 1, INFERENCE_IMAGE_CHANNELS, INFERENCE_IMAGE_WIDTH,
			   INFERENCE_IMAGE_HEIGHT };
	struct vaccel_prof_region torch_model_load_stats =
		VACCEL_PROF_REGION_INIT("torch_model_load");
	struct vaccel_prof_region torch_model_run_stats =
		VACCEL_PROF_REGION_INIT("torch_model_run");

	if (argc < 4 || argc > 5) {
		fprintf(stderr,
			"Usage: %s <image_file> <model_file> <labels_file> [iterations]\n",
			argv[0]);
		return VACCEL_EINVAL;
	}

	ret = vaccel_resource_init(&model, argv[2], VACCEL_RESOURCE_MODEL);
	if (ret != VACCEL_OK) {
		fprintf(stderr, "Could not initialize model resource\n");
		return ret;
	}

	printf("Initialized model resource %" PRId64 "\n", model.id);

	ret = vaccel_session_init(&sess, 0);
	if (ret != VACCEL_OK) {
		fprintf(stderr, "Could not initialize vAccel session\n");
		goto release_resource;
	}

	printf("Initialized vAccel session %" PRId64 "\n", sess.id);

	ret = vaccel_resource_register(&model, &sess);
	if (ret != VACCEL_OK) {
		fprintf(stderr, "Could not register model with session\n");
		goto release_session;
	}

	vaccel_prof_region_start(&torch_model_load_stats);

	ret = vaccel_torch_model_load(&sess, &model);
	if (ret != VACCEL_OK) {
		vaccel_error("Could not load torch model");
		goto unregister_resource;
	}

	vaccel_prof_region_stop(&torch_model_load_stats);

	struct vaccel_torch_tensor *in;
	ret = vaccel_torch_tensor_new(&in, 4, dims, VACCEL_TORCH_FLOAT);
	if (ret != VACCEL_OK) {
		fprintf(stderr, "Could not allocate memory\n");
		goto unregister_resource;
	}

	ret = inference_load_and_preprocess_image(argv[1],
						  INFERENCE_IMAGE_FORMAT_TORCH,
						  (float **)&in->data,
						  &in->size);
	if (ret != VACCEL_OK) {
		fprintf(stderr, "Could not load and preprocess image\n");
		goto delete_in_tensor;
	}
	in->owned = true;

	struct vaccel_torch_tensor *out = NULL;

	const int iter = (argc > 4) ? atoi(argv[4]) : 1;
	for (int i = 0; i < iter; i++) {
		vaccel_prof_region_start(&torch_model_run_stats);

		ret = vaccel_torch_model_run(&sess, &model, NULL, &in, 1, &out,
					     1);

		vaccel_prof_region_stop(&torch_model_run_stats);

		if (ret != VACCEL_OK) {
			fprintf(stderr, "Could not run op: %d\n", ret);
			goto delete_in_tensor;
		}

		printf("Success!\n");
		printf("Result Tensor :\n");
		printf("Output tensor => type:%u nr_dims:%" PRId64
		       " size:%zuB\n",
		       out->data_type, out->nr_dims, out->size);

		ret = inference_process_result(out->data, argv[3]);
		if (ret)
			fprintf(stderr, "Could not process result\n");

		if (vaccel_torch_tensor_delete(out) != VACCEL_OK) {
			fprintf(stderr, "could not delete output tensor\n");
			goto delete_in_tensor;
		}

		if (ret)
			goto delete_in_tensor;
	}

delete_in_tensor:
	if (vaccel_torch_tensor_delete(in) != VACCEL_OK)
		fprintf(stderr, "Could not delete input tensor\n");
unregister_resource:
	if (vaccel_resource_unregister(&model, &sess) != VACCEL_OK)
		fprintf(stderr, "Could not unregister model from session\n");
release_session:
	if (vaccel_session_release(&sess) != VACCEL_OK)
		fprintf(stderr, "Could not release session\n");
release_resource:
	if (vaccel_resource_release(&model) != VACCEL_OK)
		fprintf(stderr, "Could not release model resource\n");

	vaccel_prof_region_print(&torch_model_load_stats);
	vaccel_prof_region_print(&torch_model_run_stats);

	vaccel_prof_region_release(&torch_model_load_stats);
	vaccel_prof_region_release(&torch_model_run_stats);

	return ret;
}
