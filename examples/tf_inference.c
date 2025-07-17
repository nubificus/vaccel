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
	struct vaccel_prof_region tf_model_load_stats =
		VACCEL_PROF_REGION_INIT("tf_model_load");
	struct vaccel_prof_region tf_model_run_stats =
		VACCEL_PROF_REGION_INIT("tf_model_run");
	struct vaccel_prof_region tf_model_unload_stats =
		VACCEL_PROF_REGION_INIT("tf_model_unload");

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

	struct vaccel_tf_status status;
	status.message = NULL;

	vaccel_prof_region_start(&tf_model_load_stats);

	ret = vaccel_tf_model_load(&sess, &model, &status);

	vaccel_prof_region_stop(&tf_model_load_stats);

	printf("Session load status => code:%" PRIu8 " message:%s\n",
	       status.error_code, status.message);
	if (ret) {
		fprintf(stderr, "Could not load graph from model\n");
		goto unregister_resource;
	}

	if (vaccel_tf_status_release(&status))
		fprintf(stderr, "Could not release model load status\n");

	/* Input tensors & nodes */
	struct vaccel_tf_node in_node = { "serving_default_data", 0 };

	int64_t dims[] = { 1, INFERENCE_IMAGE_HEIGHT, INFERENCE_IMAGE_WIDTH,
			   INFERENCE_IMAGE_CHANNELS };

	struct vaccel_tf_tensor *in;
	ret = vaccel_tf_tensor_new(&in, 4, dims, VACCEL_TF_FLOAT);
	if (ret) {
		fprintf(stderr, "Could not create input tensor\n");
		goto delete_tf_model;
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

	/* Output tensors & nodes */
	struct vaccel_tf_node out_node = { "PartitionedCall", 0 };
	struct vaccel_tf_tensor *out = NULL;

	const int iter = (argc > 4) ? atoi(argv[4]) : 1;
	for (int i = 0; i < iter; i++) {
		vaccel_prof_region_start(&tf_model_run_stats);

		ret = vaccel_tf_model_run(&sess, &model, NULL, &in_node, &in, 1,
					  &out_node, &out, 1, &status);

		vaccel_prof_region_stop(&tf_model_run_stats);

		printf("Session run status => code:%" PRIu8 " message:%s\n",
		       status.error_code, status.message);
		if (ret) {
			fprintf(stderr, "Could not run op: %d\n", ret);
			goto release_tf_status;
		}

		printf("Success!\n");
		printf("Output tensor => type:%u nr_dims:%d size:%zuB\n",
		       out->data_type, out->nr_dims, out->size);

		ret = inference_process_result(out->data, argv[3]);
		if (ret)
			fprintf(stderr, "Could not process result\n");

		if (vaccel_tf_tensor_delete(out)) {
			fprintf(stderr, "Could not delete output tensor\n");
			goto release_tf_status;
		}

		if (ret)
			goto release_tf_status;

		if (i < (iter - 1)) {
			if (vaccel_tf_status_release(&status)) {
				fprintf(stderr,
					"Could not release model run status\n");
				goto delete_in_tensor;
			}
		}
	}

release_tf_status:
	if (vaccel_tf_status_release(&status))
		fprintf(stderr, "Could not release model run status\n");
delete_in_tensor:
	if (vaccel_tf_tensor_delete(in))
		fprintf(stderr, "Could not delete input tensor\n");
delete_tf_model:
	vaccel_prof_region_start(&tf_model_unload_stats);

	if (vaccel_tf_model_unload(&sess, &model, &status))
		fprintf(stderr, "Could not unload tf model\n");

	vaccel_prof_region_stop(&tf_model_unload_stats);

	printf("Session delete status => code:%" PRIu8 " message:%s\n",
	       status.error_code, status.message);
	if (vaccel_tf_status_release(&status))
		fprintf(stderr, "Could not release model unload status\n");
unregister_resource:
	if (vaccel_resource_unregister(&model, &sess))
		fprintf(stderr, "Could not unregister model from session\n");
release_session:
	if (vaccel_session_release(&sess))
		fprintf(stderr, "Could not release session\n");
release_resource:
	if (vaccel_resource_release(&model))
		fprintf(stderr, "Could not release resource\n");

	vaccel_prof_region_print(&tf_model_load_stats);
	vaccel_prof_region_print(&tf_model_unload_stats);
	vaccel_prof_region_print(&tf_model_run_stats);

	vaccel_prof_region_release(&tf_model_load_stats);
	vaccel_prof_region_release(&tf_model_unload_stats);
	vaccel_prof_region_release(&tf_model_run_stats);

	return ret;
}
