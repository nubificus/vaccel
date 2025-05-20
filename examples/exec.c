// SPDX-License-Identifier: Apache-2.0

#include "vaccel.h"
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

enum { INPUT_VAL = 10 };

int main(int argc, char *argv[])
{
	int ret;
	struct vaccel_session sess;
	const int input = INPUT_VAL;
	int output = 0;
	struct vaccel_prof_region mytestfunc_stats =
		VACCEL_PROF_REGION_INIT("mytestfunc");

	if (argc < 2 || argc > 3) {
		fprintf(stderr, "Usage: %s <lib_file> [iterations]\n", argv[0]);
		return VACCEL_EINVAL;
	}

	sess.hint = VACCEL_PLUGIN_DEBUG;
	ret = vaccel_session_init(&sess, sess.hint);
	if (ret) {
		fprintf(stderr, "Could not initialize session\n");
		return ret;
	}

	printf("Initialized session with id: %" PRId64 "\n", sess.id);

	struct vaccel_arg read[] = { { .size = sizeof(input),
				       .buf = (void *)&input } };
	struct vaccel_arg write[] = {
		{ .size = sizeof(output), .buf = &output },
	};

	const int iter = (argc > 2) ? atoi(argv[2]) : 1;
	for (int i = 0; i < iter; i++) {
		vaccel_prof_region_start(&mytestfunc_stats);

		ret = vaccel_exec(&sess, argv[1], "mytestfunc", read,
				  sizeof(read) / sizeof(read[0]), write,
				  sizeof(write) / sizeof(write[0]));

		vaccel_prof_region_stop(&mytestfunc_stats);

		if (ret) {
			fprintf(stderr, "Could not run op: %d\n", ret);
			goto release_session;
		}

		printf("output: %d\n", output);
	}

release_session:
	if (vaccel_session_release(&sess))
		fprintf(stderr, "Could not clear session\n");

	vaccel_prof_region_print(&mytestfunc_stats);
	vaccel_prof_region_release(&mytestfunc_stats);

	return ret;
}
