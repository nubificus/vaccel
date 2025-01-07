// SPDX-License-Identifier: Apache-2.0

#include "vaccel.h"
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>

int main()
{
	int ret;
	struct vaccel_session sess;

	float a[5] = { 5.0, 1.0, 2.1, 1.2, 5.2 };
	float b[5] = { -2.2, 1.1, 6.4, 2.3, 6.1 };
	float c[5] = { 0, 0, 0, 0, 0 };
	size_t len_a = sizeof(a) / sizeof(a[0]);
	size_t len_b = sizeof(b) / sizeof(b[1]);
	size_t len_c = (len_a > len_b ? len_a : len_b);

	ret = vaccel_session_init(&sess, 0);
	if (ret != VACCEL_OK) {
		fprintf(stderr, "Could not initialize session\n");
		return 1;
	}

	printf("Initialized session with id: %" PRId64 "\n", sess.id);
	vaccel_op_t op_type = VACCEL_F_VECTORADD;
	struct vaccel_arg read[3] = { { .size = sizeof(vaccel_op_t),
					.buf = &op_type },
				      { .size = sizeof(a), .buf = (char *)a },
				      { .size = sizeof(b), .buf = (char *)b } };
	struct vaccel_arg write[1] = {
		{ .size = (len_a > len_b ? len_a : len_b), .buf = c },
	};

	ret = vaccel_genop(&sess, read, 3, write, 1);
	if (ret) {
		fprintf(stderr, "Could not run op: %d\n", ret);
		goto close_session;
	}

	for (int i = 0; i < (int)len_c; i++) {
		printf("%f\n", c[i]);
	}

close_session:
	if (vaccel_session_release(&sess) != VACCEL_OK) {
		fprintf(stderr, "Could not clear session\n");
		return 1;
	}

	return ret;
}
