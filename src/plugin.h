// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "include/vaccel/plugin.h" // IWYU pragma: export
#include "op.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int plugin_parse_version(int *major, int *minor1, int *minor2, char **extra,
			 const char *str);
int plugin_register(struct vaccel_plugin *plugin);
int plugin_unregister(struct vaccel_plugin *plugin);
void *plugin_get_op_func(vaccel_op_type_t op_type, unsigned int hint);
size_t plugin_nr_registered();
struct vaccel_plugin *plugin_virtio();
int plugins_bootstrap();
int plugins_cleanup();

#ifdef __cplusplus
}
#endif
