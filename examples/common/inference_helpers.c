// SPDX-License-Identifier: Apache-2.0

#define _POSIX_C_SOURCE 200809L

#include "inference_helpers.h"
#include "vaccel.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef USE_STB_IMAGE
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"
#endif

int inference_load_labels(const char *filename, char ***labels,
			  size_t *nr_labels)
{
	if (!filename || !labels || !nr_labels)
		return VACCEL_EINVAL;

	int ret = VACCEL_OK;

	FILE *file = fopen(filename, "r");
	if (!file) {
		fprintf(stderr, "Failed to open %s: %s\n", filename,
			strerror(errno));
		return VACCEL_EINVAL;
	}

	size_t num_lines = 0;
	char buffer[256];

	/* First pass: Count number of lines */
	while (fgets(buffer, sizeof(buffer), file))
		num_lines++;

	if (num_lines == 0) {
		ret = VACCEL_EINVAL;
		goto out_fclose;
	}

	*labels = (char **)malloc(num_lines * sizeof(char *));
	if (!*labels) {
		fprintf(stderr, "Memory allocation for labels failed\n");
		ret = VACCEL_ENOMEM;
		goto out_fclose;
	}

	if (fseek(file, 0L, SEEK_SET)) {
		fprintf(stderr, "Could not rewind labels' file: %s\n",
			strerror(errno));
		ret = VACCEL_EIO;
		goto out_fclose;
	}

	/* Second pass: Read and store labels */
	size_t cnt = 0;
	while (cnt < num_lines && fgets(buffer, sizeof(buffer), file)) {
		buffer[strcspn(buffer, "\r\n")] = 0; // Remove newline

		(*labels)[cnt] = strdup(buffer);
		if (!(*labels)[cnt]) {
			fprintf(stderr,
				"Memory allocation for label %zu failed\n",
				cnt);

			for (size_t i = 0; i < cnt; i++)
				free((*labels)[i]);
			free(*labels);

			ret = VACCEL_ENOMEM;
			goto out_fclose;
		}
		cnt++;
	}

	*nr_labels = cnt;

out_fclose:
	fclose(file);
	return ret;
}

int inference_preprocess_image(const unsigned char *image_data, int width,
			       int height, int channels,
			       inference_image_format_t format,
			       float *preproc_data)
{
	if (channels != INFERENCE_IMAGE_CHANNELS) {
		fprintf(stderr, "Error: Only %d-channel images are supported\n",
			INFERENCE_IMAGE_CHANNELS);
		return VACCEL_EINVAL;
	}

	/* Mean and standard deviation for normalization (ImageNet) */
	const float mean[] = { 0.485F, 0.456F, 0.406F };
	const float std[] = { 0.229F, 0.224F, 0.225F };

	/* Allocate intermediate buffer for resized image */
	unsigned char resized_image[INFERENCE_IMAGE_WIDTH *
				    INFERENCE_IMAGE_HEIGHT *
				    INFERENCE_IMAGE_CHANNELS];

	/* Resize manually (nearest neighbor interpolation) */
	for (int y = 0; y < INFERENCE_IMAGE_HEIGHT; ++y) {
		for (int x = 0; x < INFERENCE_IMAGE_WIDTH; ++x) {
			int const src_x = x * width / INFERENCE_IMAGE_WIDTH;
			int const src_y = y * height / INFERENCE_IMAGE_HEIGHT;
			for (int c = 0; c < INFERENCE_IMAGE_CHANNELS; ++c) {
				resized_image[(y * INFERENCE_IMAGE_WIDTH + x) *
						      INFERENCE_IMAGE_CHANNELS +
					      c] =
					image_data[(src_y * width + src_x) *
							   channels +
						   c];
			}
		}
	}

	/* Normalize and reorder channels to specified format */
	if (format == INFERENCE_IMAGE_FORMAT_TORCH) {
		/* PyTorch format: [C, H, W] */
		for (int c = 0; c < INFERENCE_IMAGE_CHANNELS; ++c) {
			for (int y = 0; y < INFERENCE_IMAGE_HEIGHT; ++y) {
				for (int x = 0; x < INFERENCE_IMAGE_WIDTH;
				     ++x) {
					unsigned char const pixel_value = resized_image
						[(y * INFERENCE_IMAGE_WIDTH +
						  x) * INFERENCE_IMAGE_CHANNELS +
						 c];
					float const normalized_value =
						((float)pixel_value / 255.0F -
						 mean[c]) /
						std[c];
					preproc_data
						[c * INFERENCE_IMAGE_WIDTH *
							 INFERENCE_IMAGE_HEIGHT +
						 y * INFERENCE_IMAGE_WIDTH + x] =
							normalized_value;
				}
			}
		}
	} else if (format == INFERENCE_IMAGE_FORMAT_TF) {
		/* TensorFlow format: [H, W, C] */
		for (int y = 0; y < INFERENCE_IMAGE_HEIGHT; ++y) {
			for (int x = 0; x < INFERENCE_IMAGE_WIDTH; ++x) {
				for (int c = 0; c < INFERENCE_IMAGE_CHANNELS;
				     ++c) {
					unsigned char const pixel_value = resized_image
						[(y * INFERENCE_IMAGE_WIDTH +
						  x) * INFERENCE_IMAGE_CHANNELS +
						 c];
					float const normalized_value =
						((float)pixel_value / 255.0F -
						 mean[c]) /
						std[c];
					preproc_data
						[y * INFERENCE_IMAGE_WIDTH *
							 INFERENCE_IMAGE_CHANNELS +
						 x * INFERENCE_IMAGE_CHANNELS +
						 c] = normalized_value;
				}
			}
		}
	} else {
		fprintf(stderr, "Error: Unsupported format\n");
		return VACCEL_EINVAL;
	}

	return VACCEL_OK;
}

enum { RND_INPUT_MIN = 1, RND_INPUT_MAX = 100 };

int inference_generate_random_data(size_t size, float **rnd_data)
{
	if (!rnd_data)
		return VACCEL_EINVAL;

	if (!size || size % sizeof(float) != 0) {
		fprintf(stderr, "Expected an size for a float array\n");
		return VACCEL_EINVAL;
	}

	*rnd_data = (float *)malloc(size);
	if (!*rnd_data) {
		fprintf(stderr, "Memory allocation for random data failed\n");
		return VACCEL_ENOMEM;
	}

	srand((unsigned int)time(NULL));

	size_t cnt = size / sizeof(float);
	for (size_t i = 0; i < cnt; ++i)
		(*rnd_data)[i] =
			(float)(rand() % (RND_INPUT_MAX - RND_INPUT_MIN + 1) +
				RND_INPUT_MIN);

	return VACCEL_OK;
}

int inference_load_and_preprocess_image(char *img_path,
					inference_image_format_t format,
					float **preproc_data,
					size_t *preproc_size)
{
	if (!preproc_data || !preproc_size)
		return VACCEL_EINVAL;

#ifdef USE_STB_IMAGE
	int ret;

	int width;
	int height;
	int channels;
	unsigned char *img_data =
		stbi_load(img_path, &width, &height, &channels, 0);
	if (img_data == NULL) {
		fprintf(stderr, "Failed to load image\n");
		return VACCEL_EINVAL;
	}

	size_t p_img_size =
		(size_t)(INFERENCE_IMAGE_WIDTH * INFERENCE_IMAGE_HEIGHT *
			 INFERENCE_IMAGE_CHANNELS) *
		sizeof(float);
	float *p_img_data = (float *)malloc(p_img_size);
	if (!p_img_data) {
		fprintf(stderr,
			"Could not allocate memory for preprocessed data\n");
		ret = VACCEL_ENOMEM;
		goto free_img_data;
	}

	memset(p_img_data, 0, p_img_size);

	ret = inference_preprocess_image(img_data, width, height, channels,
					 format, p_img_data);
	if (ret != VACCEL_OK) {
		fprintf(stderr, "Could not preprocess image data\n");
		goto free_p_img_data;
	}

	stbi_image_free(img_data);

	*preproc_data = p_img_data;
	*preproc_size = p_img_size;

	return VACCEL_OK;

free_p_img_data:
	free(p_img_data);
free_img_data:
	stbi_image_free(img_data);
	return ret;
#else
	(void)img_path;
	(void)format;
	fprintf(stderr, "Warning: STB not found; will generate random input\n");

	float *p_img_data = NULL;
	size_t p_img_size =
		(size_t)(INFERENCE_IMAGE_WIDTH * INFERENCE_IMAGE_HEIGHT *
			 INFERENCE_IMAGE_CHANNELS) *
		sizeof(float);
	int ret = inference_generate_random_data(p_img_size, &p_img_data);
	if (ret) {
		fprintf(stderr, "Could not generate random input\n");
		return ret;
	}

	*preproc_data = p_img_data;
	*preproc_size = p_img_size;

	return VACCEL_OK;
#endif
}

int inference_process_result(const float *data, const char *labels_path)
{
	if (!data || !labels_path)
		return VACCEL_EINVAL;

#ifdef USE_STB_IMAGE
	float max = data[0];
	size_t max_idx = 0;

	for (int i = 1; i != 1000; ++i) {
		if (data[i] > max) {
			max = data[i];
			max_idx = i;
		}
	}

	char **labels;
	size_t nr_labels;
	int ret = inference_load_labels(labels_path, &labels, &nr_labels);
	if (ret) {
		fprintf(stderr, "Could not load labels from %s\n", labels_path);
		return ret;
	}
	printf("Prediction: %s\n", labels[max_idx]);

	/* Release labels' memory */
	for (size_t i = 0; i < nr_labels; ++i)
		free(labels[i]);
	free(labels);
#endif
	return VACCEL_OK;
}
