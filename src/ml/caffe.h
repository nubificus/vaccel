#ifndef __VACCEL_ML_MODEL_CAFFE_H__
#define __VACCEL_ML_MODEL_CAFFE_H__

#include <stdint.h>

#include "model.h"

struct vaccel_ml_caffe_model {
	/* This needs to be first, so we can properly cast from the base type
	 * to this type */
	struct vaccel_ml_model ml_model;

	/* Path to prototxt file */
	char *prototxt;

	/* Path to binary model file */
	char *bin_model;

	/* Path to model classes */
	char *labels;
};

int vaccel_ml_caffe_model_init(struct vaccel_ml_caffe_model *model,
		uint32_t flags, const char *prototxt, const char *bin_model,
		const char *labels);

int vaccel_ml_caffe_model_destroy(struct vaccel_ml_caffe_model *model);

#endif /* __VACCEL_ML_MODEL_CAFFE_H__ */
