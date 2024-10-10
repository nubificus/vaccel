// SPDX-License-Identifier: Apache-2.0

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <vaccel.h>

int main(int argc, char **argv)
{
	int ret;
	struct vaccel_session sess;
	int input;
	int output = 0;

	if (argc < 3) {
		vaccel_error(
			"You must specify path to shared object and the number of iterations");
		return 1;
	}
	ret = vaccel_sess_init(&sess, 0);
	if (ret != VACCEL_OK) {
		vaccel_error("Could not initialize session\n");
		return 1;
	}

	printf("Initialized session with id: %u\n", sess.session_id);

	input = 10; /* some random input value */
	enum vaccel_op_type op_type = VACCEL_EXEC;
	struct vaccel_arg read[4] = {
		{ .size = sizeof(uint8_t), .buf = &op_type },
		{ .size = strlen(argv[1]), .buf = argv[1] },
		{ .size = strlen("mytestfunc"), .buf = "mytestfunc" },
		{ .size = sizeof(input), .buf = &input }
	};
	struct vaccel_arg write[1] = {
		{ .size = sizeof(output), .buf = &output },
	};

	for (int i = 0; i < atoi(argv[2]); ++i) {
		ret = vaccel_genop(&sess, read, 4, write, 1);
		if (ret) {
			vaccel_error("Could not run op: %d\n", ret);
			goto close_session;
		}
	}

	printf("output(2x%d): %d\n", input, output);

close_session:
	if (vaccel_sess_free(&sess) != VACCEL_OK) {
		vaccel_error("Could not clear session\n");
		return 1;
	}

	return ret;
}
