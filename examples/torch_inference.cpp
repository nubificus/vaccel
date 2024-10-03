// SPDX-License-Identifier: Apache-2.0

#include <cstdio>
#include <cstdlib>
#include <fcntl.h>
#include <session.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <vaccel.h>
extern "C" {
#include "../src/utils.h"
}
#include <cstddef>
#include <cstring>
#include <iostream>
#include <random>
#include <vector>

auto random_input_generator(int min_value = 1, int max_value = 100,
			    size_t vector_size = 150528,
			    bool is_print = true) -> std::vector<float>
{
	// Create a random number generator engine
	std::random_device rd;
	std::mt19937 rng(rd());

	std::uniform_int_distribution<int> distribution(min_value, max_value);

	// Create a vector and fill it with random numbers
	std::vector<float> res_data(vector_size);
	for (size_t i = 0; i < vector_size; ++i) {
		res_data[i] = distribution(rng);
	}

	if (is_print) {
		// Print the vector contents
		std::cout << "The first Random numbers:";
		for (int value : res_data) {
			std::cout << " " << value;
			break;
		}
		std::cout << '\n';
	}
	return res_data;
}

auto main(int argc, char **argv) -> int
{
	struct vaccel_session sess;
	struct vaccel_resource model;
	struct vaccel_torch_buffer run_options;
	int ret;
	char *model_path = argv[2];
	std::vector<float> res_data = random_input_generator();
	res_data.resize(static_cast<size_t>(3 * 224 * 224));
	int64_t dims[] = { 1, 224, 224, 3 };
	struct vaccel_torch_tensor *in;
	struct vaccel_torch_tensor *out;

	if (argc != 3) {
		fprintf(stderr, "Usage: %s image filename, model path\n",
			argv[0]);
		exit(1);
	}

	ret = vaccel_resource_new(&model, model_path, VACCEL_FILE_DATA);
	if (ret != 0) {
		vaccel_error("Could not create model resource");
		return ret;
	}

	/* Read the image file */
	ret = read_file(argv[1], (void **)&run_options.data, &run_options.size);
	if (ret != VACCEL_OK) {
		fprintf(stderr, "Could not load the image file: %d", ret);
		goto close_session;
	}

	ret = vaccel_sess_init(&sess, 0);
	if (ret != VACCEL_OK) {
		fprintf(stderr, "Could not initialize vAccel session\n");
		goto destroy_resource;
	}

	printf("Initialized vAccel session %u\n", sess.session_id);

	/* Take vaccel_session and vaccel_resource as inputs, 
	 * register a resource with a session */
	ret = vaccel_resource_register(&sess, &model);
	if (ret != VACCEL_OK) {
		fprintf(stderr, "Could not register model with session\n");
		goto close_session;
	}

	in = vaccel_torch_tensor_new(4, dims, VACCEL_TORCH_FLOAT);
	if (in == nullptr) {
		fprintf(stderr, "Could not allocate memory\n");
		goto unregister_session;
	}
	in->data = res_data.data();
	in->size = res_data.size() * sizeof(float);
	printf("The IN ADDRESS: %p\n", in->data);
	printf("data: %f\n", *(float *)in->data);
	printf("size: %zu\n", in->size);

	/* Output tensor */
	out = (struct vaccel_torch_tensor *)malloc(
		sizeof(struct vaccel_torch_tensor) * 1);
	if (out == nullptr) {
		fprintf(stderr, "Could not allocate memory\n");
		goto free_tensor;
	}

	/* Conducting torch inference */
	ret = vaccel_torch_jitload_forward(&sess, &model, &run_options, &in, 1,
					   &out, 1);
	if (ret != VACCEL_OK) {
		fprintf(stderr, "Could not run op: %d\n", ret);
		goto free_tensor;
	}

	printf("Success!\n");
	printf("Result Tensor :\n");
	printf("Output tensor => type:%u nr_dims:%u\n", out->data_type,
	       out->nr_dims);
	printf("data: %f\n", *(float *)out->data);
	printf("size: %zu\n", out->size);
	/*
	   const char* classes[10] = { "plane", "car", "bird", 
	   "cat", "deer", "dog", 
	   "frog", "horse", "ship", "truck" };

	   printf("Pred: %s\n", classes[int(offsets)])
	   */

	if (vaccel_torch_tensor_destroy(out) != VACCEL_OK)
		fprintf(stderr, "Could not destroy out tensor\n");
free_tensor:
	if (vaccel_torch_tensor_destroy(in) != VACCEL_OK)
		fprintf(stderr, "Could not destroy in tensor\n");
unregister_session:
	if (vaccel_resource_unregister(&sess, &model) != VACCEL_OK)
		fprintf(stderr, "Could not unregister model with session\n");
close_session:
	if (vaccel_sess_free(&sess) != VACCEL_OK)
		fprintf(stderr, "Could not clear session\n");
destroy_resource:
	free(run_options.data);
	vaccel_resource_destroy(&model);

	return ret;
}
