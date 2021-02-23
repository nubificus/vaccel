#ifndef __VACCEL_ML_MODEL_H__
#define __VACCEL_ML_MODEL_H__

#include <stdint.h>

#include "resources.h"

struct vaccel_session;

struct vaccel_ml_model {
	/* vAccel resource descriptor */
	struct vaccel_resource resource;

	/* vAccelRT-related flags */
	uint32_t flags;
};

struct vaccel_ml_model_ops {
	/* Register model */
	int (*register_model)(struct vaccel_ml_model *model, struct vaccel_session *session);

	/* Unregister model */
	int (*unregister_model)(struct vaccel_ml_model *model);
};

int vaccel_ml_model_init(struct vaccel_ml_model *model, uint32_t flags);

#endif /* __VACCEL_ML_MODEL_H__ */
