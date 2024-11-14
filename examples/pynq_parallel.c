// SPDX-License-Identifier: Apache-2.0

#include <stdio.h>
#include <stdlib.h>

#include <vaccel.h>

int main()
{
	/* Doesnt work unless HLS but the other repo has better implementation */

	int ret;
	struct vaccel_session sess;
	int N = 2;

	float a[] = { 1, 2, 3, 4 };
	float b[] = { 2, 3, 4, 5 };
	float c[N * N];
	float d[N * N];

	size_t len_a = sizeof(a) / sizeof(a[0]);

	ret = vaccel_session_init(&sess, 0);
	if (ret != VACCEL_OK) {
		fprintf(stderr, "Could not initialize session\n");
		return 1;
	}

	printf("Initialized session with id: %u\n", sess.session_id);

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
	if (vaccel_session_free(&sess) != VACCEL_OK) {
		fprintf(stderr, "Could not clear session\n");
		return 1;
	}

	return ret;
}
