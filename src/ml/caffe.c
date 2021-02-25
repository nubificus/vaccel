#include <string.h>

#include "caffe.h"
#include "common.h"
#include "session.h"

int vaccel_ml_caffe_model_init(struct vaccel_ml_caffe_model *model,
		uint32_t flags, const char *prototxt, const char *bin_model,
		const char *labels)
{
	if (!model)
		return VACCEL_EINVAL;

	int ret = vaccel_ml_model_init(&model->ml_model, flags);
	if (ret != VACCEL_OK)
		return ret;

	model->prototxt = strdup(prototxt);
	if (!model->prototxt)
		return VACCEL_ENOMEM;

	model->bin_model = strdup(bin_model);
	if (!model->bin_model)
		goto free_prototxt;

	model->labels = strdup(labels);
	if (!model->labels)
		goto free_bin_model;

	return VACCEL_OK;

free_bin_model:
	free(model->bin_model);

free_prototxt:
	free(model->prototxt);

	return VACCEL_ENOMEM;
}

int vaccel_ml_caffe_model_destroy(struct vaccel_ml_caffe_model *model)
{
	if (!model)
		return VACCEL_EINVAL;

	free(model->prototxt);
	free(model->bin_model);
	free(model->labels);

	return VACCEL_OK;
}
