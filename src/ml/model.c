#include "model.h"
#include "session.h"
#include "vaccel.h"
#include "resources.h"
#include "plugin.h"
#include "common.h"
#include "log.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

int vaccel_ml_model_init(struct vaccel_ml_model *model, uint32_t flags)
{
	if (!model)
		return VACCEL_EINVAL;

	model->flags = flags;

	return VACCEL_OK;
}

int vaccel_ml_model_register(
	struct vaccel_session *sess,
	enum vaccel_resource_type type,
	struct vaccel_ml_model *model
) {
	if (!model)
		return VACCEL_EINVAL;

	if (!sess)
		return VACCEL_EINVAL;

	struct vaccel_plugin *virtio = get_virtio_plugin();
	if (virtio)
		return virtio->info->register_resource(sess, type,
				&model->resource);

	/* TODO: Check here if we need to offload the registration to the
	 * VirtIO module */
	int ret = session_register_resource(sess, &model->resource);
	if (ret != VACCEL_OK)
		return ret;

	return VACCEL_OK;
}

int vaccel_ml_model_unregister(struct vaccel_ml_model *model)
{
	if (!model)
		return VACCEL_EINVAL;

	struct vaccel_session *sess =
		(struct vaccel_session *) model->resource.owner;

	if (!sess)
		return VACCEL_ENOENT;

	struct vaccel_plugin *virtio = get_virtio_plugin();
	if (virtio)
		return virtio->info->unregister_resource(sess,
				&model->resource);

	/* TODO: Check here if we need to offload the registration to the
	 * VirtIO module */
	int ret = session_unregister_resource(sess, &model->resource);
	if (ret != VACCEL_OK)
		return ret;

	return VACCEL_OK;

}
