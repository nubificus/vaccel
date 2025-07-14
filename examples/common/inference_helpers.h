// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <stddef.h>

enum {
	INFERENCE_IMAGE_WIDTH = 224,
	INFERENCE_IMAGE_HEIGHT = 224,
	INFERENCE_IMAGE_CHANNELS = 3
};

typedef enum {
	INFERENCE_IMAGE_FORMAT_TORCH,
	INFERENCE_IMAGE_FORMAT_TF
} inference_image_format_t;

#ifdef __cplusplus
extern "C" {
#endif

int inference_load_labels(const char *filename, char ***labels,
			  size_t *nr_labels);
int inference_preprocess_image(const unsigned char *image_data, int width,
			       int height, int channels,
			       inference_image_format_t format,
			       float *preproc_data);
int inference_generate_random_data(size_t size, float **rnd_data);
int inference_load_and_preprocess_image(char *img_path,
					inference_image_format_t format,
					float **preproc_data,
					size_t *preproc_size);
int inference_process_result(const float *data, const char *labels_path);

#ifdef __cplusplus
}
#endif
