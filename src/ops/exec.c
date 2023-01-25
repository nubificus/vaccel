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

#include "exec.h"
#include "error.h"
#include "plugin.h"
#include "log.h"
#include "vaccel_ops.h"
#include "genop.h"

#include "session.h"

int vaccel_exec(struct vaccel_session *sess, const char *library,
				const char *fn_symbol, struct vaccel_arg *read,
				size_t nr_read, struct vaccel_arg *write, size_t nr_write)
{
	if (!sess)
		return VACCEL_EINVAL;

	vaccel_debug("session:%u Looking for plugin implementing exec",
				 sess->session_id);

	// Get implementation
	int (*plugin_op)() = get_plugin_op(VACCEL_EXEC);
	if (!plugin_op)
		return VACCEL_ENOTSUP;

	return plugin_op(sess, library, fn_symbol, read, nr_read,
					 write, nr_write);
}

int vaccel_exec_with_resource(struct vaccel_session *sess, struct vaccel_shared_object *object,
							const char *fn_symbol, struct vaccel_arg *read,
							size_t nr_read, struct vaccel_arg *write, size_t nr_write)
{
	if (!sess)
		return VACCEL_EINVAL;

	vaccel_debug("session:%u Looking for plugin implementing exec",
				 sess->session_id);

	// Get implementation
	int (*plugin_op)() = get_plugin_op(VACCEL_EXEC_WITH_RESOURCE);
	if (!plugin_op)
		return VACCEL_ENOTSUP;

	return plugin_op(sess, object, fn_symbol, read, nr_read,
					 write, nr_write);
}

int vaccel_exec_unpack(struct vaccel_session *sess,
		struct vaccel_arg *read, int nr_read,
		struct vaccel_arg *write, int nr_write)
{
	if (nr_read < 2) {
		vaccel_error("Wrong number of read arguments in exec: %d",
				nr_read);
		return VACCEL_EINVAL;
	}

	/* Pop the first two arguments */
	char *library = (char *)read[0].buf;
	char *fn_symbol = (char *)read[1].buf;

	/* Pass on the rest of the read and all write arguments */
	return vaccel_exec (sess, library, fn_symbol, &read[2],
			nr_read - 2, write, nr_write);
}
