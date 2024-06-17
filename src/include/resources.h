// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <stddef.h>

#include "error.h"
#include "vaccel_id.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	VACCEL_RES_SHARED_OBJ = 0,
	VACCEL_RES_SINGLE_MODEL,
	VACCEL_RES_TF_SAVED_MODEL,
	VACCEL_RES_MAX
} vaccel_resource_t;

struct vaccel_resource;

int vaccel_resource_get_deps(struct vaccel_resource ***deps, size_t *nr_deps,
		struct vaccel_resource *res);
int vaccel_resource_deps_to_ids(vaccel_id_t *ids, struct vaccel_resource **res,
		size_t nr_ids);
int vaccel_resource_deps_from_ids(struct vaccel_resource **deps,
		vaccel_id_t *ids, size_t nr_ids);
int vaccel_resource_set_deps_from_ids(struct vaccel_resource *res,
		vaccel_id_t *ids, size_t nr_ids);

#ifdef __cplusplus
}
#endif
