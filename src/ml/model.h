#ifndef __VACCEL_ML_MODEL_H__
#define __VACCEL_ML_MODEL_H__

#include <stdint.h>

struct vaccel_ml_model;
struct vaccel_session;

int vaccel_ml_register_model(struct vaccel_ml_model *model,
		struct vaccel_session *session, const char *path,
		uint32_t flags);
int vaccel_ml_unregister_model(struct vaccel_ml_model *model);

#endif /* __VACCEL_ML_MODEL_H__ */
