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
#include "genop.h"
#include "log.h"
#include "plugin.h"
#include "session.h"
#include "vaccel_ops.h"
#include "vaccel_prof.h"

struct vaccel_prof_region noop_op_stats =
	VACCEL_PROF_REGION_INIT("vaccel_noop_op");

int vaccel_noop(struct vaccel_session *sess)
{
	int ret;

	if (!sess)
		return VACCEL_EINVAL;

	vaccel_debug("session:%u Looking for plugin implementing noop",
		     sess->session_id);

	vaccel_prof_region_start(&noop_op_stats);

	//Get implementation
	int (*plugin_op)() = get_plugin_op(VACCEL_NO_OP, sess->hint);
	if (!plugin_op)
		return VACCEL_ENOTSUP;

	ret = plugin_op(sess);

	vaccel_prof_region_stop(&noop_op_stats);

	return ret;
}

int vaccel_noop_unpack(struct vaccel_session *sess, struct vaccel_arg *read,
		       int nr_read, struct vaccel_arg *write, int nr_write)
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

__attribute__((constructor)) static void vaccel_ops_init(void)
{
}

__attribute__((destructor)) static void vaccel_ops_fini(void)
{
	vaccel_prof_region_print(&noop_op_stats);
}
