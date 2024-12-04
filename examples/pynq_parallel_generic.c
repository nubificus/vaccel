// SPDX-License-Identifier: Apache-2.0

#include "vaccel.h"
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>

enum { N = 2 };

int main()
{
	/* Doesnt work unless HLS but the other repo has better implementation */

	int ret;
	struct vaccel_session sess;

	float a[N * N] = { 1, 2, 3, 4 };
	float b[N * N] = { 2, 3, 4, 5 };
	float c[N * N];
	float d[N * N];

	size_t len_a = sizeof(a) / sizeof(a[0]);

	ret = vaccel_session_init(&sess, 0);
	if (ret != VACCEL_OK) {
		fprintf(stderr, "Could not initialize session\n");
		return 1;
	}

	printf("Initialized session with id: %" PRId64 "\n", sess.id);

	enum vaccel_op_type op_type = VACCEL_F_PARALLEL;
	struct vaccel_arg read[3] = {
		{ .size = sizeof(enum vaccel_op_type), .buf = &op_type },
		{ .size = sizeof(a), .buf = (char *)a },
		{ .size = sizeof(b), .buf = (char *)b },
	};
	struct vaccel_arg write[2] = {
		{ .size = sizeof(a), .buf = c },
		{ .size = sizeof(a), .buf = d },
	};

	//for (int i = 0; i < atoi(argv[2]); ++i) {
	ret = vaccel_genop(&sess, read, 3, write, 2);
	if (ret) {
		fprintf(stderr, "Could not run op: %d\n", ret);
		goto close_session;
	}
	printf("C: ");
	for (int i = 0; i < N * N; i++) {
		printf("%f\n", c[i]);
	}
	printf("D: ");
	for (int i = 0; i < N * N; i++) {
		printf("%f\n", d[i]);
	}

	ret = vaccel_fpga_parallel(&sess, a, b, c, d, len_a);
	if (ret) {
		fprintf(stderr, "Could not run op: %d\n", ret);
		goto close_session;
	}

	for (int i = 0; i < (int)len_a; i++) {
		printf("%f\n", c[i]);
	}

	for (int i = 0; i < (int)len_a; i++) {
		printf("%f\n", d[i]);
	}

close_session:
	if (vaccel_session_release(&sess) != VACCEL_OK) {
		fprintf(stderr, "Could not clear session\n");
		return 1;
	}

	return ret;
}
