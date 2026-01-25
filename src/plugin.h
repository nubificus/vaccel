// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "include/vaccel/plugin.h" // IWYU pragma: export
#include "list.h"
#include "op.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int plugins_bootstrap();
int plugins_cleanup();
int plugin_parse_version(int *major, int *minor1, int *minor2, char **extra,
			 const char *str);
int plugin_register(struct vaccel_plugin *plugin);
int plugin_unregister(struct vaccel_plugin *plugin);
struct vaccel_plugin *plugin_find(unsigned int hint);
struct vaccel_plugin *plugin_find_by_name(const char *name);
void *plugin_get_op_func(struct vaccel_plugin *plugin,
			 vaccel_op_type_t op_type);
size_t plugin_count();

/* Helper macros for iterating lists of containers */
#define plugin_for_each(iter, list) \
	list_for_each_container((iter), (list), struct vaccel_plugin, entry)

#define plugin_for_each_safe(iter, tmp, list)               \
	list_for_each_container_safe((iter), (tmp), (list), \
				     struct vaccel_plugin, entry)

#ifdef __cplusplus
}
#endif
