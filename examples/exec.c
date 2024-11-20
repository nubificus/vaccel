// SPDX-License-Identifier: Apache-2.0

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <vaccel.h>

int main(int argc, char *argv[])
{
	int ret;
	struct vaccel_session sess;
	int input = 0;
	int output = 0;

	if (argc < 3) {
		vaccel_error(
			"You must specify path to shared object and the number of iterations");
		return 1;
	}

	sess.hint = VACCEL_PLUGIN_DEBUG;
	ret = vaccel_session_init(&sess, sess.hint);
	if (ret != VACCEL_OK) {
		vaccel_error("Could not initialize session");
		return 1;
	}

	printf("Initialized session with id: %" PRId64 "\n", sess.id);

	input = 10; /* some random input value */
	struct vaccel_arg read[1] = { { .size = sizeof(input),
					.buf = &input } };
	struct vaccel_arg write[1] = {
		{ .size = sizeof(output), .buf = &output },
	};

	for (int i = 0; i < atoi(argv[2]); ++i) {
		ret = vaccel_exec(&sess, argv[1], "mytestfunc", read, 1, write,
				  1);
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
