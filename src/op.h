// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "include/vaccel/op.h" // IWYU pragma: export
#include "inttypes.h"
#include "log.h"
#include "session.h"
#include "utils/enum.h"

static inline void op_debug_plugin_lookup(const struct vaccel_session *sess,
					  vaccel_op_type_t op_type)
{
	char op_name[VACCEL_ENUM_STR_MAX];
	vaccel_debug(
		"session:%" PRId64 " Looking for plugin implementing op %s",
		sess->id,
		vaccel_op_type_name(op_type, op_name, VACCEL_ENUM_STR_MAX));
}
