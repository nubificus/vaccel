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

#include "noop.h"
#include "error.h"
#include "plugin.h"
#include "log.h"
#include "vaccel_ops.h"
#include "genop.h"

#include "session.h"

int vaccel_noop(struct vaccel_session *sess)
{
	if (!sess)
		return VACCEL_EINVAL;

	vaccel_debug("session:%u Looking for plugin implementing noop",
			sess->session_id);

	//Get implementation
	int (*plugin_op)() = get_plugin_op(VACCEL_NO_OP, sess->hint);
	if (!plugin_op)
		return VACCEL_ENOTSUP;

	return plugin_op(sess);
}

int vaccel_noop_unpack(struct vaccel_session *sess,
		struct vaccel_arg *read, int nr_read,
		struct vaccel_arg *write, int nr_write)
{
	if (nr_read || read) {
		vaccel_error("Wrong number of read arguments in noop: %d",
				nr_read);
		return VACCEL_EINVAL;
	}

	if (nr_write || write) {
		vaccel_error("Wrong number of write arguments in noop: %d",
				nr_write);
		return VACCEL_EINVAL;
	}

	return vaccel_noop(sess);
}
