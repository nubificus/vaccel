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

#include <stdio.h>

#include "misc.h"
#include "error.h"
#include "plugin.h"
#include "log.h"

#include "session.h"

int vaccel_get_plugins(struct vaccel_session *sess, enum vaccel_op_type op_type)
{
	if (!sess)
		return VACCEL_EINVAL;

	vaccel_debug("session:%u Query for plugins implementing %s",
				 sess->session_id, vaccel_op_type_str(op_type));

	int ret = get_available_plugins(op_type);

	return ret;
}
