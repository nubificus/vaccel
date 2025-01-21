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

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb/stb_image_resize.h"

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
constexpr int total_elems = target_width * target_height
					 * target_channels;
const int nr_req_args = 4;
const std::string usage_args = "image-path model-path labels-path";

auto main(int argc, char **argv) -> int
{
	if (argc < nr_req_args) {
		std::cerr << "Usage: " << argv[0] << " " << usage_args << '\n';
		return VACCEL_EINVAL;
	}

	struct vaccel_session sess;
	struct vaccel_resource model;
	int ret;
	int dims[] = { 1, 224, 224, 3 };
	int dims_resp[] = { 1, 1000 };
	int channel_idx[] = { 0, 0 };
	int permute_as[] = { 0, 3, 1, 2 };
	
	/* The tensors */
	vaccel_tensor_t image;
	vaccel_tensor_t resp;
	unsigned char *resized_data = nullptr;
	float *img_float = nullptr;
	
	/* Result */
	unsigned int max_idx;
	float response_ptr[1000] = { 0 };
	std::vector<std::string> labels;

	float to_sub;
	float to_div;

	ret = vaccel_resource_init(&model, argv[2], VACCEL_RESOURCE_MODEL);
	if (ret != 0) {
		fprintf(stderr, "Could not create model resource\n");
		return ret;
	}

	printf("Initialized vAccel resource %" PRId64 "\n", model.id);

	ret = vaccel_session_init(&sess, 0);
	if (ret != VACCEL_OK) {
		fprintf(stderr, "Could not initialize vAccel session\n");
		goto destroy_resource;
	}

	printf("Initialized vAccel session %" PRId64 "\n", sess.id);

	ret = vaccel_resource_register(&model, &sess);
	if (ret != VACCEL_OK) {
		fprintf(stderr, "Could not register model with session\n");
		goto close_session;
	}

	if (!load_labels(argv[3], labels)) {
		fprintf(stderr, "Could not load labels from %s", argv[3]);
		return VACCEL_ENOENT;
	}

	int width;
	int height;
	int channels;
	unsigned char *img_data;

	img_data = stbi_load(argv[1], &width, &height, &channels, 0);
	if (img_data == nullptr) {
		std::cerr << "Failed to load image\n";
		ret = VACCEL_EINVAL;
		goto unregister_session;
	}

	resized_data = new unsigned char[(size_t)total_elems];
	if (resized_data == nullptr) {
		fprintf(stderr, "Could not allocate memory to process data");
		stbi_image_free(img_data);
		ret = VACCEL_ENOMEM;
		goto unregister_session;
	}

	if (!stbir_resize_uint8(img_data, width, height, 0, resized_data,
				target_width, target_height, 0, target_channels)) {
		fprintf(stderr, "Error: Failed to resize image\n");
		delete[] resized_data;
		stbi_image_free(img_data);
		goto unregister_session;
	}

	stbi_image_free(img_data);

	img_float = new float[(size_t) total_elems];
	if (img_float == nullptr) {
		fprintf(stderr, "Error: Failed to transform image to float\n");
		delete[] resized_data;
		stbi_image_free(img_data);
		goto unregister_session;
	}

	for (size_t i = 0; i < total_elems; ++i)
		img_float[i] = (float)resized_data[i];

	ret = vaccel_tensor_init(&sess, &image, 4, dims,
				 img_float, VACCEL_FLOAT);
	if (ret) {
		vaccel_error("Could not initialize image tensor");
		exit(0);
	}

	vaccel_tensor_permute(&sess, &image, 4, permute_as);

	/* Scale to [0, 1] */
	to_div = 255.0f;
	vaccel_tensor_div_val(&sess, &image, &to_div, VACCEL_FLOAT);

	vaccel_tensor_t channel;
	vaccel_tensor_get_sub(&sess, &image, &channel, 2, channel_idx);

	to_sub = 0.485;
	to_div = 0.229;
	vaccel_tensor_sub_val(&sess, &channel, &to_sub, VACCEL_FLOAT);
	vaccel_tensor_div_val(&sess, &channel, &to_div, VACCEL_FLOAT);

	channel_idx[1] = 1;
	vaccel_tensor_get_sub(&sess, &image, &channel, 2, channel_idx);
	
	to_sub = 0.456;
	to_div = 0.224;
	vaccel_tensor_sub_val(&sess, &channel, &to_sub, VACCEL_FLOAT);
	vaccel_tensor_div_val(&sess, &channel, &to_div, VACCEL_FLOAT);
	
	channel_idx[1] = 2;
	vaccel_tensor_get_sub(&sess, &image, &channel, 2, channel_idx);

	to_sub = 0.406;
	to_div = 0.225;
	vaccel_tensor_sub_val(&sess, &channel, &to_sub, VACCEL_FLOAT);
	vaccel_tensor_div_val(&sess, &channel, &to_div, VACCEL_FLOAT);

	ret = vaccel_tensor_alloc(&sess, &resp, 2, dims_resp,
				  VACCEL_FLOAT);
	if (ret) {
		vaccel_error("Coud not initialize response tensor");
		exit(0);
	}

	ret = vaccel_tensor_forward(&sess, &model, 1, &image, &resp);
	if (ret) {
		vaccel_error("Could not run forward function");
		exit(0);
	}

	ret = vaccel_tensor_get_data(&sess, &resp, response_ptr);
	if (ret) {
		vaccel_error("Could not read the data from the response tensor");
		exit(0);
	}

	/* Conducting torch inference */
	max_idx = 0;

	for (auto i = 1; i != 1000; ++i)
		if (response_ptr[i] > response_ptr[max_idx])
			max_idx = i;

	std::cout << "Prediction: " << labels[max_idx] << '\n';

unregister_session:
	delete[] resized_data;

	if (vaccel_resource_unregister(&model, &sess) != VACCEL_OK)
		fprintf(stderr, "Could not unregister model with session\n");
close_session:
	delete[] img_float;

	if (vaccel_session_release(&sess) != VACCEL_OK)
		fprintf(stderr, "Could not clear session\n");
destroy_resource:
	vaccel_resource_release(&model);

	return ret;
}
