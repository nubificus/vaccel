// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "include/vaccel/resource.h" // IWYU pragma: export
#include "list.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct resource_registration;

int resources_bootstrap(void);
int resources_cleanup(void);
void resource_refcount_inc(struct vaccel_resource *res);
void resource_refcount_dec(struct vaccel_resource *res);
int resource_create_rundir(struct vaccel_resource *res);
void resource_destroy_rundir(struct vaccel_resource *res);
int resource_unregister_from_registration(struct resource_registration *reg);

/* Helper macros for iterating lists of containers */
#define resource_for_each(iter, list) \
	list_for_each_container((iter), (list), struct vaccel_resource, entry)

#define resource_for_each_safe(iter, tmp, list)             \
	list_for_each_container_safe((iter), (tmp), (list), \
				     struct vaccel_resource, entry)

#ifdef __cplusplus
}
#endif
