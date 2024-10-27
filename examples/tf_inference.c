// SPDX-License-Identifier: Apache-2.0

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

#include <ops/tf.h>
#include <vaccel.h>

#define min(a, b) ((a) < (b) ? (a) : (b))

int main(int argc, char *argv[])
{
	struct vaccel_session vsess;
	struct vaccel_resource model;

	if (argc != 2) {
		vaccel_error("usage: %s model", argv[0]);
		exit(1);
	}

	char *model_path = argv[1];

	int ret =
		vaccel_resource_init(&model, model_path, VACCEL_RESOURCE_MODEL);
	if (ret) {
		vaccel_error("Could not create model resource");
		exit(1);
	}

	printf("Created new model %" PRId64 "\n", model.id);

	ret = vaccel_session_init(&vsess, 0);
	if (ret) {
		vaccel_error("Could not initialize vAccel session");
		goto destroy_resource;
	}

	printf("Initialized vAccel session %" PRId64 "\n", vsess.id);

	ret = vaccel_resource_register(&model, &vsess);
	if (ret) {
		vaccel_error("Could not register model with session");
		goto close_session;
	}

	struct vaccel_tf_status status;
	ret = vaccel_tf_session_load(&vsess, &model, &status);
	if (ret) {
		vaccel_error("Could not load graph from model");
		goto unregister_resource;
	}
	if (status.message)
		free((char *)status.message);

	struct vaccel_tf_buffer run_options = { NULL, 0 };

	/* Input tensors & nodes */
	struct vaccel_tf_node in_node = { "serving_default_input_1", 0 };

	int64_t dims[] = { 1, 30 };
	float data[30];
	for (int i = 0; i < 30; ++i)
		data[i] = 1.00;

	struct vaccel_tf_tensor *in =
		vaccel_tf_tensor_new(2, dims, VACCEL_TF_FLOAT);
	if (!in) {
		vaccel_error("Could not allocate memory");
		exit(1);
	}

	in->data = data;
	in->size = sizeof(float) * 30;

	/* Output tensors & nodes */
	struct vaccel_tf_node out_node = { "StatefulPartitionedCall", 0 };
	struct vaccel_tf_tensor *out;

	ret = vaccel_tf_session_run(&vsess, &model, &run_options, &in_node, &in,
				    1, &out_node, &out, 1, &status);
	if (ret) {
		vaccel_error("Could not run model: %d", ret);
		goto unload_session;
	}

	printf("Success!\n");
	printf("Output tensor => type:%u nr_dims:%u\n", out->data_type,
	       out->nr_dims);
	for (int i = 0; i < out->nr_dims; ++i)
		printf("dim[%d]: %" PRId64 "\n", i, out->dims[i]);

	printf("Result Tensor :\n");
	float *offsets = (float *)out->data;
	for (unsigned int i = 0; i < min(10, out->size / sizeof(float)); ++i)
		printf("%f\n", offsets[i]);

	if (status.message)
		free((char *)status.message);
	if (vaccel_tf_tensor_destroy(out))
		vaccel_error("Could not destroy out tensor");
unload_session:
	if (vaccel_tf_tensor_destroy(in))
		vaccel_error("Could not destroy in tensor");

	if (vaccel_tf_session_delete(&vsess, &model, &status))
		vaccel_error("Could not delete tf session");
	if (status.message)
		free((char *)status.message);
unregister_resource:
	if (vaccel_resource_unregister(&model, &vsess))
		vaccel_error("Could not unregister model with session");
close_session:
	if (vaccel_session_release(&vsess))
		vaccel_error("Could not clear session");
destroy_resource:
	vaccel_resource_release(&model);

	return ret;
}
