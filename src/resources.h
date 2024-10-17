// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "include/resources.h"
#include "id_pool.h"
#include "list.h"
#include <stdatomic.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct vaccel_resource;

int resources_bootstrap(void);
int resources_cleanup(void);
int resource_new(struct vaccel_resource *res, vaccel_resource_t type,
		 void *data, int (*cleanup_resource)(void *));
int resource_get_by_id(struct vaccel_resource **resource, vaccel_id_t id);
int resource_set_deps(struct vaccel_resource *res,
		      struct vaccel_resource **deps, size_t nr_deps);
int resource_unset_deps(struct vaccel_resource *res);
int resource_destroy(struct vaccel_resource *res);
void resource_refcount_inc(struct vaccel_resource *res);
void resource_refcount_dec(struct vaccel_resource *res);
int resource_create_rundir(struct vaccel_resource *res);
int resource_destroy_rundir(struct vaccel_resource *res);

/* Helper macros for iterating lists of containers */
#define for_each_vaccel_resource(iter, list) \
	for_each_container((iter), (list), struct vaccel_resource, entry)

#define for_each_vaccel_resource_safe(iter, tmp, list)                         \
	for_each_container_safe((iter), (tmp), (list), struct vaccel_resource, \
				entry)

#ifdef __cplusplus
}
#endif
