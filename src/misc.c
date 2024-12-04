// SPDX-License-Identifier: Apache-2.0

#include "misc.h"
#include "error.h"
#include "log.h"
#include "ops/vaccel_ops.h"
#include "plugin.h"
#include "session.h"
#include <inttypes.h>
#include <stdint.h>

int vaccel_get_plugins(struct vaccel_session *sess, enum vaccel_op_type op_type)
{
	if (!sess)
		return VACCEL_EINVAL;

	vaccel_debug("session:%" PRId64 " Query for plugins implementing %s",
		     sess->id, vaccel_op_type_str(op_type));

	int ret = get_available_plugins(op_type);

	return ret;
}
