// SPDX-License-Identifier: Apache-2.0

#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <random>
#include <session.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

#ifdef USE_STB_IMAGE
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"
#endif

#include <vaccel.h>

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
		res_data[i] = (float)distribution(rng);
	}

	if (is_print) {
		// Print the vector contents
		std::cout << "The first Random numbers:";
		for (float value : res_data) {
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
	int64_t dims[] = { 1, 224, 224, 3 };
	struct vaccel_torch_tensor *in;
	struct vaccel_torch_tensor *out;
	std::vector<float> res_data;

	if (argc != 3) {
		fprintf(stderr, "Usage: %s image filename, model path\n",
			argv[0]);
		exit(1);
	}

	ret = vaccel_resource_init(&model, model_path, VACCEL_RESOURCE_MODEL);
	if (ret != 0) {
		fprintf(stderr, "Could not create model resource\n");
		return ret;
	}

	ret = vaccel_session_init(&sess, 0);
	if (ret != VACCEL_OK) {
		fprintf(stderr, "Could not initialize vAccel session\n");
		goto destroy_resource;
	}

	printf("Initialized vAccel session %" PRId64 "\n", sess.id);

	/* Take vaccel_session and vaccel_resource as inputs, 
	 * register a resource with a session */
	ret = vaccel_resource_register(&model, &sess);
	if (ret != VACCEL_OK) {
		fprintf(stderr, "Could not register model with session\n");
		goto close_session;
	}

#ifdef USE_STB_IMAGE
	int width;
	int height;
	int channels;
	unsigned char *img_data;

	img_data = stbi_load(argv[1], &width, &height, &channels, 0);
	if (img_data == nullptr) {
		std::cerr << "Failed to load image\n";
		return -1;
	}

	res_data.reserve((size_t)width * (size_t)height * (size_t)channels);

	for (int i = 0; i < width * height * channels; ++i) {
		res_data.push_back((float)img_data[i] /
				   255.0F); // Normalize pixel value
	}
	stbi_image_free(img_data);
#else
	res_data = random_input_generator();
	res_data.resize((size_t)(224 * 224 * 3));
#endif

	in = vaccel_torch_tensor_new(4, dims, VACCEL_TORCH_FLOAT);
	if (in == nullptr) {
		fprintf(stderr, "Could not allocate memory\n");
		goto unregister_session;
	}

	in->data = res_data.data();
	in->size = res_data.size() * sizeof(float);

	run_options.data = strdup("resnet");
	run_options.size = strlen(run_options.data) + 1;

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
	free(run_options.data);
	if (vaccel_torch_tensor_destroy(in) != VACCEL_OK)
		fprintf(stderr, "Could not destroy in tensor\n");
unregister_session:
	if (vaccel_resource_unregister(&model, &sess) != VACCEL_OK)
		fprintf(stderr, "Could not unregister model with session\n");
close_session:
	if (vaccel_session_release(&sess) != VACCEL_OK)
		fprintf(stderr, "Could not clear session\n");
destroy_resource:
	vaccel_resource_release(&model);

	return ret;
}
