// SPDX-License-Identifier: Apache-2.0

#include "vaccel.h"
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>

int main(int argc, char *argv[])
{
	int ret;
	struct vaccel_session sess;

	if (argc != 2) {
		fprintf(stderr, "Usage: %s <plugin>\n", argv[0]);
		fprintf(stderr,
			"  <plugin>  plugin name (e.g. \"noop\") or, for "
			"transport plugins,\n"
			"            \"local:remote\" (e.g. \"rpc:noop\")\n");
		return VACCEL_EINVAL;
	}

	ret = vaccel_session_init_with_plugin(&sess, argv[1]);
	if (ret) {
		fprintf(stderr, "Could not initialize session with %s\n",
			argv[1]);
		return ret;
	}
	printf("Initialized session with id %" PRId64 " using plugin '%s'\n",
	       sess.id, argv[1]);

	ret = vaccel_noop(&sess);
	if (ret)
		fprintf(stderr, "Could not run op: %d\n", ret);

	if (vaccel_session_release(&sess))
		fprintf(stderr, "Could not release session\n");

	return ret;
}
