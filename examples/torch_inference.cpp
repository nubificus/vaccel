// SPDX-License-Identifier: Apache-2.0

#include <cinttypes>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <random>
#include <string>
#include <string.h>
#include <vector>

#include <session.h>

#ifdef USE_STB_IMAGE
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"
#endif

#include "vaccel.h"

auto load_labels(const std::string &file_name,
		 std::vector<std::string> &labels) -> bool
{
	std::ifstream ifs(file_name);
	if (!ifs)
		return false;

	std::string line;
	while (std::getline(ifs, line))
		labels.push_back(line);

	return true;
}

const int target_width = 224;
const int target_height = 224;
const int target_channels = 3;

auto preprocess(const unsigned char *image_data, int width, int height,
		int channels, float *output_buffer) -> int
{
	if (channels != 3) {
		std::cerr << "Error: Only 3-channel RGB images are supported\n";
		return VACCEL_EINVAL;
	}

	/* Mean and standard deviation for normalization */
	const float mean[] = { 0.485F, 0.456F, 0.406F };
	const float std[] = { 0.229F, 0.224F, 0.225F };

	/* Allocate intermediate buffer for resized image */
	std::vector<unsigned char> resized_image(
		(size_t)(target_width * target_height * 3));

	/* Resize manually (nearest neighbor interpolation) */
	for (int y = 0; y < target_height; ++y) {
		for (int x = 0; x < target_width; ++x) {
			int const src_x = x * width / target_width;
			int const src_y = y * height / target_height;
			for (int c = 0; c < 3; ++c) {
				resized_image[(y * target_width + x) * 3 + c] =
					image_data[(src_y * width + src_x) * 3 +
						   c];
			}
		}
	}

	/* Normalize and reorder channels to [C, H, W] */
	for (int c = 0; c < 3; ++c) {
		for (int y = 0; y < target_height; ++y) {
			for (int x = 0; x < target_width; ++x) {
				unsigned char const pixel_value =
					resized_image[(y * target_width + x) * 3 +
						      c];
				float const normalized_value =
					((float)pixel_value / 255.0F -
					 mean[c]) /
					std[c];
				output_buffer[c * target_width * target_height +
					      y * target_width + x] =
					normalized_value;
			}
		}
	}
	return VACCEL_OK;
}

enum { RND_INPUT_MIN = 1, RND_INPUT_MAX = 100, VEC_SIZE_DEFAULT = 150528 };

auto generate_random_input(int min_value = RND_INPUT_MIN,
			   int max_value = RND_INPUT_MAX,
			   size_t vector_size = VEC_SIZE_DEFAULT,
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
		for (float const value : res_data) {
			std::cout << " " << value;
			break;
		}
		std::cout << '\n';
	}
	return res_data;
}

#ifdef USE_STB_IMAGE
const int nr_req_args = 4;
const std::string usage_args = "<image_file> <model_file> <labels_file>";
#else
const int nr_req_args = 3;
const std::string usage_args = "<image_file> <model_file>";
#endif

auto main(int argc, char **argv) -> int
{
	struct vaccel_session sess;
	struct vaccel_resource model;
	struct vaccel_torch_buffer run_options;
	int ret;
	int64_t dims[] = { 1, target_channels, target_width, target_height };
	struct vaccel_torch_tensor *in;
	struct vaccel_torch_tensor *out;
	struct vaccel_prof_region torch_jitload_forward_stats =
		VACCEL_PROF_REGION_INIT("torch_jitload_forward");

	std::vector<float> res_data;
#ifdef USE_STB_IMAGE
	float *preprocessed_data = nullptr;
	float max;
	unsigned int max_idx;
	float *response_ptr;
	std::vector<std::string> labels;
#endif

	if (argc < nr_req_args || argc > (nr_req_args + 1)) {
		std::cerr << "Usage: " << argv[0] << " " << usage_args
			  << " [iterations]\n";
		return VACCEL_EINVAL;
	}

	const int iter = (argc > nr_req_args) ? atoi(argv[nr_req_args]) : 1;

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

	ret = vaccel_torch_tensor_new(&in, 4, dims, VACCEL_TORCH_FLOAT);
	if (ret != VACCEL_OK) {
		fprintf(stderr, "Could not allocate memory\n");
		goto unregister_resource;
	}

#ifdef USE_STB_IMAGE
	if (!load_labels(argv[3], labels)) {
		fprintf(stderr, "Could not load labels from %s", argv[3]);
		ret = VACCEL_ENOENT;
		goto unregister_resource;
	}

	int width;
	int height;
	int channels;
	unsigned char *img_data;

	img_data = stbi_load(argv[1], &width, &height, &channels, 0);
	if (img_data == nullptr) {
		std::cerr << "Failed to load image\n";
		ret = VACCEL_EINVAL;
		goto unregister_resource;
	}

	preprocessed_data = new float[(size_t)(target_width * target_height *
					       target_channels)];
	if (preprocessed_data == nullptr) {
		fprintf(stderr, "Could not allocate memory to process data");
		stbi_image_free(img_data);
		ret = VACCEL_ENOMEM;
		goto unregister_resource;
	}

	memset(preprocessed_data, 0,
	       (size_t)(target_width * target_height * target_channels) *
		       sizeof(float));

	ret = preprocess(img_data, width, height, channels, preprocessed_data);
	if (ret != VACCEL_OK) {
		fprintf(stderr, "Could not preprocess image data");
		stbi_image_free(img_data);
		goto unregister_resource;
	}

	stbi_image_free(img_data);

	in->data = (void *)preprocessed_data;
	in->size = (size_t)(target_width * target_height * target_channels) *
		   sizeof(float);
#else
	res_data = generate_random_input();
	res_data.resize(
		(size_t)(target_width * target_height * target_channels));

	in->data = res_data.data();
	in->size = res_data.size() * sizeof(float);
#endif

	run_options.data = strdup("mobilenet");
	run_options.size = strlen(run_options.data) + 1;

	for (int i = 0; i < iter; i++) {
		vaccel_prof_region_start(&torch_jitload_forward_stats);

		ret = vaccel_torch_jitload_forward(&sess, &model, &run_options,
						   &in, 1, &out, 1);

		vaccel_prof_region_stop(&torch_jitload_forward_stats);

		if (ret != VACCEL_OK) {
			fprintf(stderr, "Could not run op: %d\n", ret);
			goto delete_in_tensor;
		}

		printf("Success!\n");
		printf("Result Tensor :\n");
		printf("Output tensor => type:%u nr_dims:%" PRId64 "\n",
		       out->data_type, out->nr_dims);
		printf("size: %zu B\n", out->size);

#ifdef USE_STB_IMAGE
		response_ptr = reinterpret_cast<float *>(out->data);
		max = response_ptr[0];
		max_idx = 0;

		for (auto i = 1; i != 1000; ++i) {
			if (response_ptr[i] > max) {
				max = response_ptr[i];
				max_idx = i;
			}
		}

		std::cout << "Prediction: " << labels[max_idx] << '\n';
#endif

		if (vaccel_torch_tensor_delete(out) != VACCEL_OK) {
			fprintf(stderr, "could not delete output tensor\n");
			goto delete_in_tensor;
		}
	}

delete_in_tensor:
	free(run_options.data);
	if (vaccel_torch_tensor_delete(in) != VACCEL_OK)
		fprintf(stderr, "Could not delete input tensor\n");
unregister_resource:
#ifdef USE_STB_IMAGE
	delete[] preprocessed_data;
#endif
	if (vaccel_resource_unregister(&model, &sess) != VACCEL_OK)
		fprintf(stderr, "Could not unregister model from session\n");
release_session:
	if (vaccel_session_release(&sess) != VACCEL_OK)
		fprintf(stderr, "Could not release session\n");
release_resource:
	if (vaccel_resource_release(&model) != VACCEL_OK)
		fprintf(stderr, "Could not release model resource\n");

	vaccel_prof_region_print(&torch_jitload_forward_stats);
	vaccel_prof_region_release(&torch_jitload_forward_stats);

	return ret;
}
