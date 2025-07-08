// SPDX-License-Identifier: Apache-2.0

#define _POSIX_C_SOURCE 200809L

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef USE_STB_IMAGE
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"
#endif

#include "vaccel.h"

size_t load_labels(const char *filename, char ***labels)
{
	FILE *file = fopen(filename, "r");
	if (!file) {
		vaccel_error("Failed to open file");
		return 0;
	}

	size_t num_lines = 0;
	char buffer[256];

	/* First pass: Count number of lines */
	while (fgets(buffer, sizeof(buffer), file))
		num_lines++;

	if (num_lines == 0) {
		fclose(file);
		return 0;
	}

	*labels = (char **)malloc(num_lines * sizeof(char *));
	if (!*labels) {
		vaccel_error("Memory allocation failed");
		fclose(file);
		return 0;
	}

	rewind(file);

	/* Second pass: Read and store labels */
	size_t count = 0;
	while (count < num_lines && fgets(buffer, sizeof(buffer), file)) {
		buffer[strcspn(buffer, "\r\n")] = 0; // Remove newline

		(*labels)[count] = strdup(buffer);
		if (!(*labels)[count]) {
			vaccel_error("Memory allocation for label failed");

			for (size_t i = 0; i < count; i++)
				free((*labels)[i]);
			free(*labels);

			fclose(file);
			return 0;
		}
		count++;
	}

	fclose(file);
	return count;
}

const int target_width = 224;
const int target_height = 224;
const int target_channels = 3;

int preprocess(const unsigned char *image_data, int width, int height,
	       int channels, float *output_buffer)
{
	if (channels != 3) {
		fprintf(stderr,
			"Error: Only 3-channel RGB images are supported\n");
		return VACCEL_EINVAL;
	}

	/* Mean and standard deviation for normalization */
	const float mean[] = { 0.485F, 0.456F, 0.406F };
	const float std[] = { 0.229F, 0.224F, 0.225F };

	/* Allocate intermediate buffer for resized image */
	unsigned char
		resized_image[target_width * target_height * target_channels];

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

float *generate_random_input()
{
	int min_value = RND_INPUT_MIN;
	int max_value = RND_INPUT_MAX;
	int vector_size = VEC_SIZE_DEFAULT;
	float *res_data = (float *)malloc(vector_size * sizeof(float));
	if (!res_data) {
		vaccel_error("Memory allocation failed");
		return NULL;
	}

	srand((unsigned int)time(NULL));

	for (int i = 0; i < vector_size; ++i)
		res_data[i] = (float)(rand() % (max_value - min_value + 1) +
				      min_value);

	return res_data;
}

#ifdef USE_STB_IMAGE
const int nr_req_args = 4;
const char *usage_args = "<image_file> <model_file> <labels_file>";
#else
const int nr_req_args = 3;
const char *usage_args = "<image_file> <model_file>";
#endif

int main(int argc, char **argv)
{
	struct vaccel_session sess;
	struct vaccel_resource model;
	struct vaccel_torch_buffer run_options;
	int ret;
	int64_t dims[] = { 1, target_channels, target_width, target_height };
	struct vaccel_torch_tensor *in;
	struct vaccel_torch_tensor *out;
	struct vaccel_prof_region torch_model_load_stats =
		VACCEL_PROF_REGION_INIT("torch_model_load");
	struct vaccel_prof_region torch_model_run_stats =
		VACCEL_PROF_REGION_INIT("torch_model_run");
#ifdef USE_STB_IMAGE
	float *preprocessed_data = NULL;
	float max;
	unsigned int max_idx;
	float *response_ptr;
	char **labels;
	size_t nr_labels;
#else
	float *res_data;
#endif

	if (argc < nr_req_args || argc > (nr_req_args + 1)) {
		fprintf(stderr, "Usage: %s %s [iterations]\n", argv[0],
			usage_args);
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

	vaccel_prof_region_start(&torch_model_load_stats);

	ret = vaccel_torch_model_load(&sess, &model);
	if (ret != VACCEL_OK) {
		vaccel_error("Could not load torch model");
		goto unregister_resource;
	}

	vaccel_prof_region_stop(&torch_model_load_stats);

	ret = vaccel_torch_tensor_new(&in, 4, dims, VACCEL_TORCH_FLOAT);
	if (ret != VACCEL_OK) {
		fprintf(stderr, "Could not allocate memory\n");
		goto unregister_resource;
	}

#ifdef USE_STB_IMAGE
	int width;
	int height;
	int channels;
	unsigned char *img_data;

	img_data = stbi_load(argv[1], &width, &height, &channels, 0);
	if (img_data == NULL) {
		fprintf(stderr, "Failed to load image\n");
		ret = VACCEL_EINVAL;
		goto unregister_resource;
	}

	preprocessed_data = (float *)malloc(
		(size_t)(target_width * target_height * target_channels) *
		sizeof(float));
	if (preprocessed_data == NULL) {
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
	if (res_data == NULL) {
		fprintf(stderr, "Could not generate random input\n");
		ret = VACCEL_ENOMEM;
		goto delete_in_tensor;
	}
	in->data = res_data;
	in->size = VEC_SIZE_DEFAULT * sizeof(float);
#endif

	run_options.data = strdup("classify");
	run_options.size = strlen(run_options.data) + 1;

	for (int i = 0; i < iter; i++) {
		vaccel_prof_region_start(&torch_model_run_stats);

		ret = vaccel_torch_model_run(&sess, &model, &run_options, &in,
					     1, &out, 1);

		vaccel_prof_region_stop(&torch_model_run_stats);

		if (ret != VACCEL_OK) {
			fprintf(stderr, "Could not run op: %d\n", ret);
			goto free_res_data;
		}

		printf("Success!\n");
		printf("Result Tensor :\n");
		printf("Output tensor => type:%u nr_dims:%" PRId64 "\n",
		       out->data_type, out->nr_dims);
		printf("size: %zu B\n", out->size);

#ifdef USE_STB_IMAGE
		response_ptr = (float *)out->data;
		max = response_ptr[0];
		max_idx = 0;

		for (int i = 1; i != 1000; ++i) {
			if (response_ptr[i] > max) {
				max = response_ptr[i];
				max_idx = i;
			}
		}
		nr_labels = load_labels(argv[3], &labels);
		if (!nr_labels) {
			fprintf(stderr, "Could not load labels from %s\n",
				argv[3]);
		} else {
			printf("Prediction: %s\n", labels[max_idx]);
			/* Release labels memory */
			for (size_t i = 0; i < nr_labels; ++i)
				free(labels[i]);
			free(labels);
		}
#endif

		if (vaccel_torch_tensor_delete(out) != VACCEL_OK) {
			fprintf(stderr, "could not delete output tensor\n");
			goto free_res_data;
		}
	}

free_res_data:
#ifndef USE_STB_IMAGE
	free(res_data);
delete_in_tensor:
#endif
	free(run_options.data);
	if (vaccel_torch_tensor_delete(in) != VACCEL_OK)
		fprintf(stderr, "Could not delete input tensor\n");
unregister_resource:
#ifdef USE_STB_IMAGE
	free(preprocessed_data);
#endif
	if (vaccel_resource_unregister(&model, &sess) != VACCEL_OK)
		fprintf(stderr, "Could not unregister model from session\n");
release_session:
	if (vaccel_session_release(&sess) != VACCEL_OK)
		fprintf(stderr, "Could not release session\n");
release_resource:
	if (vaccel_resource_release(&model) != VACCEL_OK)
		fprintf(stderr, "Could not release model resource\n");

	vaccel_prof_region_print(&torch_model_load_stats);
	vaccel_prof_region_release(&torch_model_load_stats);

	vaccel_prof_region_print(&torch_model_run_stats);
	vaccel_prof_region_release(&torch_model_run_stats);
	return ret;
}
