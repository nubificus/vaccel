/*
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

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
