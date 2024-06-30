// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "include/plugin.h"
#include "ops/vaccel_ops.h"
#include "vaccel.h"
#include <stdint.h>

void *get_plugin_op(enum vaccel_op_type op_type, unsigned int hint);
int get_available_plugins(enum vaccel_op_type op_type);
struct vaccel_plugin *get_virtio_plugin(void);
int plugins_bootstrap(void);
int plugins_shutdown(void);
