#include <string.h>

#include "caffe.h"
#include "common.h"
#include "session.h"

static int register_model(struct vaccel_ml_model *model,
		struct vaccel_session *sess)
{
	if (!model)
		return VACCEL_EINVAL;

	if (!sess)
		return VACCEL_EINVAL;

	struct vaccel_ml_caffe_model *caffe =
		(struct vaccel_ml_caffe_model *) model;

	/* TODO: Check here if we need to offload the registration to the
	 * VirtIO module */
	int ret = session_register_resource(sess, &caffe->ml_model.resource);
	if (ret != VACCEL_OK)
		return ret;

	return VACCEL_OK;
}

static int unregister_model(struct vaccel_ml_model *model)
{
	if (!model)
		return VACCEL_EINVAL;

	struct vaccel_ml_caffe_model *caffe =
		(struct vaccel_ml_caffe_model *) model;

	struct vaccel_session *sess =
		(struct vaccel_session *) model->resource.owner;

	if (!sess)
		return VACCEL_ENOENT;

	/* TODO: Check here if we need to offload the registration to the
	 * VirtIO module */
	int ret = session_unregister_resource(sess, &caffe->ml_model.resource);
	if (ret != VACCEL_OK)
		return ret;

	return VACCEL_OK;
}

int vaccel_ml_caffe_model_init(struct vaccel_ml_caffe_model *model,
		uint32_t flags, const char *prototxt, const char *bin_model,
		const char *labels)
{
	if (!model)
		return VACCEL_EINVAL;

	int ret = vaccel_ml_model_init(&model->ml_model, flags);
	if (ret != VACCEL_OK)
		return ret;

	model->ops.register_model = register_model;
	model->ops.unregister_model = unregister_model;

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
