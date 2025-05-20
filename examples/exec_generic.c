// SPDX-License-Identifier: Apache-2.0

#include "vaccel.h"
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { INPUT_VAL = 10 };

int main(int argc, char **argv)
{
	int ret;
	struct vaccel_session sess;
	const int input = INPUT_VAL;
	int output;
	char *func = "mytestfunc";
	struct vaccel_prof_region mytestfunc_stats =
		VACCEL_PROF_REGION_INIT("mytestfunc");

	if (argc < 2 || argc > 3) {
		fprintf(stderr, "Usage: %s <lib_file> [iterations]\n", argv[0]);
		return VACCEL_EINVAL;
	}

	ret = vaccel_session_init(&sess, 0);
	if (ret) {
		fprintf(stderr, "Could not initialize session\n");
		return ret;
	}

	printf("Initialized session with id: %" PRId64 "\n", sess.id);

	vaccel_op_type_t op_type = VACCEL_OP_EXEC;
	struct vaccel_arg read[] = {
		{ .size = sizeof(uint8_t), .buf = &op_type, .argtype = 0 },
		{ .size = strlen(argv[1]) + 1, .buf = argv[1], .argtype = 0 },
		{ .size = strlen(func) + 1, .buf = func, .argtype = 0 },
		{ .size = sizeof(input), .buf = (void *)&input, .argtype = 0 }
	};
	struct vaccel_arg write[] = {
		{ .size = sizeof(output), .buf = &output, .argtype = 0 },
	};

	const int iter = (argc > 2) ? atoi(argv[2]) : 1;
	for (int i = 0; i < iter; i++) {
		vaccel_prof_region_start(&mytestfunc_stats);

		ret = vaccel_genop(&sess, read, sizeof(read) / sizeof(read[0]),
				   write, sizeof(write) / sizeof(write[0]));

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
