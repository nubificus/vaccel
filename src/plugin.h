// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "include/plugin.h"
#include "ops/vaccel_ops.h"
#include "vaccel.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void *get_plugin_op(enum vaccel_op_type op_type, unsigned int hint);
int get_available_plugins(enum vaccel_op_type op_type);
size_t get_nr_plugins();
struct vaccel_plugin *get_virtio_plugin();
int plugins_bootstrap();
int plugins_shutdown();
int parse_plugin_version(int *major, int *minor1, int *minor2, char **extra,
			 const char *str);

#ifdef __cplusplus
}
#endif
