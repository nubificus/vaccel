// SPDX-License-Identifier: Apache-2.0

#include "vaccel.h"
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

enum { N = 5 };

int main(int argc, char *argv[])
{
	int ret;
	struct vaccel_session sess;
	struct vaccel_prof_region fpga_vadd_stats =
		VACCEL_PROF_REGION_INIT("fpga_vadd");

	float a[N] = { 5.0, 1.0, 2.1, 1.2, 5.2 };
	float b[N] = { -2.2, 1.1, 6.4, 2.3, 6.1 };
	float c[N] = { 0, 0, 0, 0, 0 };
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

	const int iter = (argc > 1) ? atoi(argv[1]) : 1;
	for (int i = 0; i < iter; i++) {
		vaccel_prof_region_start(&fpga_vadd_stats);

		ret = vaccel_fpga_vadd(&sess, a, b, c, array_len, array_len);

		vaccel_prof_region_stop(&fpga_vadd_stats);

		if (ret) {
			fprintf(stderr, "Could not run op: %d\n", ret);
			goto release_session;
		}

		for (size_t i = 0; i < array_len; i++)
			printf("%f\n", c[i]);
	}

release_session:
	if (vaccel_session_release(&sess))
		fprintf(stderr, "Could not release session\n");

	vaccel_prof_region_print(&fpga_vadd_stats);
	vaccel_prof_region_release(&fpga_vadd_stats);

	return ret;
}
