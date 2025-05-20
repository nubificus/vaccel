// SPDX-License-Identifier: Apache-2.0

#include "vaccel.h"
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

enum { N = 6 };

int main(int argc, char *argv[])
{
	int ret;
	struct vaccel_session sess;
	struct vaccel_prof_region fpga_arraycopy_stats =
		VACCEL_PROF_REGION_INIT("fpga_arraycopy");

	int a[N] = { 12, 15, 12, 15, 12, 11 };
	int b[N] = { 0, 0, 0, 0, 0, 0 };
	size_t array_len = (size_t)N;

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

	vaccel_op_type_t op_type = VACCEL_OP_FPGA_ARRAYCOPY;
	struct vaccel_arg read[] = {
		{ .size = sizeof(vaccel_op_type_t),
		  .buf = &op_type,
		  .argtype = 0 },
		{ .size = sizeof(a), .buf = (char *)a, .argtype = 0 }
	};
	struct vaccel_arg write[] = {
		{ .size = sizeof(b), .buf = b, .argtype = 0 },
	};

	const int iter = (argc > 1) ? atoi(argv[1]) : 1;
	for (int i = 0; i < iter; i++) {
		vaccel_prof_region_start(&fpga_arraycopy_stats);

		ret = vaccel_genop(&sess, read, sizeof(read) / sizeof(read[0]),
				   write, sizeof(write) / sizeof(write[0]));

		vaccel_prof_region_stop(&fpga_arraycopy_stats);

		if (ret) {
			fprintf(stderr, "Could not run op: %d\n", ret);
			goto release_session;
		}

		for (size_t i = 0; i < array_len; i++)
			printf("%i\n", b[i]);
	}

release_session:
	if (vaccel_session_release(&sess))
		fprintf(stderr, "Could not release session\n");

	vaccel_prof_region_print(&fpga_arraycopy_stats);
	vaccel_prof_region_release(&fpga_arraycopy_stats);

	return ret;
}
