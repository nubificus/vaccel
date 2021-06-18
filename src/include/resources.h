#pragma once

#include "vaccel_id.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	VACCEL_RES_TF_MODEL = 0,
	VACCEL_RES_TF_SAVED_MODEL,
	VACCEL_RES_CAFFE_MODEL,
	VACCEL_RES_MAX
} vaccel_resource_t;

struct vaccel_resource;

vaccel_id_t resource_get_id(struct vaccel_resource *resource);

#ifdef __cplusplus
}
#endif
