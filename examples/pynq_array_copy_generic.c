// SPDX-License-Identifier: Apache-2.0

#include "vaccel.h"
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>

int main()
{
	int ret;
	struct vaccel_session sess;

	int a[6] = { 12, 15, 12, 15, 12, 11 };
	int b[6];
	//size_t len_a = sizeof(a) / sizeof(a[0]);
	//size_t len_b = sizeof(b)/sizeof(b[1]);

	ret = vaccel_session_init(&sess, 0);
	if (ret != VACCEL_OK) {
		fprintf(stderr, "Could not initialize session\n");
		return 1;
	}

	printf("Initialized session with id: %" PRId64 "\n", sess.id);

	vaccel_op_type_t op_type = VACCEL_OP_FPGA_ARRAYCOPY;
	struct vaccel_arg read[2] = { { .size = sizeof(vaccel_op_type_t),
					.buf = &op_type },
				      { .size = sizeof(a), .buf = (char *)a } };
	struct vaccel_arg write[2] = {
		{ .size = sizeof(b), .buf = b },
	};

	//for (int i = 0; i < atoi(argv[2]); ++i) {
	ret = vaccel_genop(&sess, read, 2, write, 1);
	if (ret) {
		fprintf(stderr, "Could not run op: %d\n", ret);
		goto close_session;
	}

#if 0
	ret = vaccel_fpga_arraycopy(&sess, a, b, len_a);
	if (ret) {
		fprintf(stderr, "Could not run op: %d\n", ret);
		goto close_session;
	}
#endif

	for (int i = 0; i < 6; i++) {
		printf("%i\n", b[i]);
	}

close_session:
	if (vaccel_session_release(&sess) != VACCEL_OK) {
		fprintf(stderr, "Could not clear session\n");
		return 1;
	}

	return ret;
}
