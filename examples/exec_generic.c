// SPDX-License-Identifier: Apache-2.0

#include "vaccel.h"
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv)
{
	int ret;
	struct vaccel_session sess;
	int input;
	int output = 0;
	char *func = "mytestfunc";

	if (argc < 3) {
		vaccel_error(
			"You must specify path to shared object and the number of iterations");
		return 1;
	}
	ret = vaccel_session_init(&sess, 0);
	if (ret != VACCEL_OK) {
		vaccel_error("Could not initialize session");
		return 1;
	}

	printf("Initialized session with id: %" PRId64 "\n", sess.id);

	input = 10; /* some random input value */
	vaccel_op_type_t op_type = VACCEL_OP_EXEC;
	struct vaccel_arg read[4] = {
		{ .size = sizeof(uint8_t), .buf = &op_type },
		{ .size = strlen(argv[1]) + 1, .buf = argv[1] },
		{ .size = strlen(func) + 1, .buf = func },
		{ .size = sizeof(input), .buf = &input }
	};
	struct vaccel_arg write[1] = {
		{ .size = sizeof(output), .buf = &output },
	};

	for (int i = 0; i < atoi(argv[2]); ++i) {
		ret = vaccel_genop(&sess, read, 4, write, 1);
		if (ret) {
			vaccel_error("Could not run op: %d", ret);
			goto close_session;
		}
	}

	printf("output: %d\n", output);

close_session:
	if (vaccel_session_release(&sess) != VACCEL_OK) {
		vaccel_error("Could not clear session");
		return 1;
	}

	return ret;
}
