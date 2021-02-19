#include "model.h"
#include "session.h"
#include "vaccel.h"
#include "resources.h"
#include "common.h"
#include "log.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

struct vaccel_ml_model {
	struct vaccel_resource resource;

	char *path;

	uint32_t flags;
};

int vaccel_ml_register_model(struct vaccel_ml_model *model,
		struct vaccel_session *session, const char *path,
		uint32_t flags)
{
	int ret;

	if (!model)
		return VACCEL_EINVAL;

	if (!session)
		return VACCEL_EINVAL;

	if (!path)
		return VACCEL_EINVAL;

	ret = vaccel_resource_new(&model->resource, VACCEL_ML_MODEL, session);
	if (ret != VACCEL_OK)
		return ret;

	model->path = strdup(path);
	if (!model->path) {
		vaccel_resource_destroy(&model->resource);
		return VACCEL_ENOMEM;
	}

	model->flags = flags;

	ret = session_register_resource(session, &model->resource);
	if (ret != VACCEL_OK)
		free(model->path);

	return ret;
}

int vaccel_ml_unregister_model(struct vaccel_ml_model *model)
{
	if (!model)
		return VACCEL_EINVAL;

	struct vaccel_resource *resource = &model->resource;

	struct vaccel_session *sess = (struct vaccel_session *)resource->owner;
	if (!sess) {
		vaccel_warn("Model %u not registered", resource->id);
		return VACCEL_EINVAL;
	}

	if (!model->path)
		vaccel_warn("Model %u without valid model path", resource->id);
	else
		free(model->path);

	int ret = session_unregister_resource(sess, resource);
	if (ret != VACCEL_OK)
		return ret;

	free(model->path);
	vaccel_resource_destroy(resource);

	return VACCEL_OK;
}
