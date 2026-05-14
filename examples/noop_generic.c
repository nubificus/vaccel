// SPDX-License-Identifier: Apache-2.0

#include "vaccel.h"
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
	int ret;
	struct vaccel_session sess;
	struct vaccel_prof_region noop_stats = VACCEL_PROF_REGION_INIT("noop");

	if (argc > 2) {
		fprintf(stderr, "Usage: %s [iterations]\n", argv[0]);
		return VACCEL_EINVAL;
	}

	ret = vaccel_session_init(&sess, 0);
	if (ret) {
		fprintf(stderr, "Could not initialize session\n");
		return ret;
	}

	printf("Initialized session with id: %" PRId64 "\n", sess.id);

	struct vaccel_arg_array read_args;
	ret = vaccel_arg_array_init(&read_args, 1);
	if (ret) {
		fprintf(stderr, "Could not initialize read args array\n");
		goto release_session;
	}

	const uint8_t op_type = (uint8_t)VACCEL_OP_NOOP;
	ret = vaccel_arg_array_add_uint8(&read_args, (uint8_t *)&op_type);
	if (ret) {
		fprintf(stderr, "Failed to pack op_type arg\n");
		goto release_read_args_array;
	}

	const int iter = (argc > 1) ? atoi(argv[1]) : 1;
	for (int i = 0; i < iter; i++) {
		vaccel_prof_region_start(&noop_stats);

		ret = vaccel_genop(&sess, read_args.args, read_args.count, NULL,
				   0);

		vaccel_prof_region_stop(&noop_stats);

		if (ret) {
			fprintf(stderr, "Could not run op: %d\n", ret);
			goto release_read_args_array;
		}
	}

release_read_args_array:
	if (vaccel_arg_array_release(&read_args))
		fprintf(stderr, "Could not release read args array\n");
release_session:
	if (vaccel_session_release(&sess))
		fprintf(stderr, "Could not release session\n");

	vaccel_prof_region_print(&noop_stats);
	vaccel_prof_region_release(&noop_stats);

	return ret;
}
