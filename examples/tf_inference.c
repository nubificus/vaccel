#include <stdlib.h>
#include <stdio.h>

#include <vaccel.h>
#include <ops/tf.h>

#define min(a, b) ((a) < (b) ? (a) : (b))

int main(int argc, char *argv[])
{
	struct vaccel_session vsess;
	struct vaccel_tf_saved_model model;

	if (argc != 2) {
		fprintf(stderr, "usage: %s model\n", argv[0]);
		exit(1);
	}

	const char *model_path = argv[1];

	int ret = vaccel_tf_saved_model_set_path(&model, model_path);
	if (ret) {
		fprintf(stderr, "Could not set model path to TensorFlow model\n");
		exit(1);
	}

	ret = vaccel_tf_saved_model_register(&model);
	if (ret) {
		fprintf(stderr, "Could not register TensorFlow model resource\n");
		exit(1);
	}

	printf("Created new model %ld\n", vaccel_tf_saved_model_id(&model));
	
	ret = vaccel_sess_init(&vsess, 0);
	if (ret) {
		fprintf(stderr, "Could not initialize vAccel session\n");
		exit(1);
	}

	printf("Initialized vAccel session %u\n", vsess.session_id);

	ret = vaccel_sess_register(&vsess, model.resource);
	if (ret) {
		fprintf(stderr, "Could not register model with session\n");
		goto close_session;
	}

	struct vaccel_tf_status status;
	ret = vaccel_tf_model_load_graph(&vsess, &model, &status);
	if (ret) {
		fprintf(stderr, "Could not load graph from model\n");
		goto unregister_resource;
	}

	struct vaccel_tf_buffer run_options = { NULL, 0 };

	/* Input tensors & nodes */
	struct vaccel_tf_node in_node = { "serving_default_input_1", 0 };
	
	int64_t dims[] = {1, 30};
	float data[30];
	for (int i = 0; i < 30; ++i)
		data[i] = 1.00;

	struct vaccel_tf_tensor *in = vaccel_tf_tensor_new(2, dims, VACCEL_TF_FLOAT);
	if (!in) {
		fprintf(stderr, "Could not allocate memory\n");
		exit(1);
	}

	in->data = data;
	in->size = sizeof(float) * 30;

	/* Output tensors & nodes */
	struct vaccel_tf_node out_node = { "StatefulPartitionedCall", 0 };
	struct vaccel_tf_tensor *out;

	ret = vaccel_tf_model_run(&vsess, &model, &run_options,
			&in_node, &in, 1,
			&out_node, &out, 1,
			&status);
	if (ret) {
		fprintf(stderr, "Could not run model: %d\n", ret);
		goto unregister_resource;
	}

	printf("Success!\n");
	printf("Output tensor => type:%u nr_dims:%u\n", out->data_type, out->nr_dims);
	for (int i = 0; i < out->nr_dims; ++i)
		printf("dim[%d]: %ld\n", i, out->dims[i]);
		
	printf("Result Tensor :\n");
	float *offsets = (float *)out->data;
	for (unsigned int i = 0; i < min(10, out->size / sizeof(float)) ; ++i)
		printf("%f\n", offsets[i]);

unregister_resource:
	vaccel_sess_unregister(&vsess, model.resource);

close_session:
	vaccel_sess_free(&vsess);

	return 0;
}
