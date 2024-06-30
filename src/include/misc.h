// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "plugin.h"
#include "ops/vaccel_ops.h"
#include "session.h"

int vaccel_get_plugins(struct vaccel_session *sess,
		       enum vaccel_op_type op_type);
