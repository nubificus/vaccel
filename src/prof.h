// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "include/vaccel/prof.h" // IWYU pragma: export
#include "include/vaccel/op.h"

int vaccel_prof_region_stop_with_context(const struct vaccel_prof_region *region,
					 vaccel_op_type_t op_type,
					 const char *plugin_name);
