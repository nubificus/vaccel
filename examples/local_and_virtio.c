// SPDX-License-Identifier: Apache-2.0

#include "vaccel.h"
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { INPUT_VAL = 10, ARGTYPE = 42 };

int main(int argc, char *argv[])
{
	if (argc != 3) {
		vaccel_error("usage: %s path_shared_object iterations",
			     argv[0]);
		return 0;
	}

	int input;
	int output1;
	struct vaccel_session virtio_sess;
	struct vaccel_session local_sess;
	struct vaccel_session *sess;
	struct vaccel_resource object;
	int ret;

	ret = vaccel_resource_init(&object, argv[1], VACCEL_RESOURCE_LIB);
	if (ret) {
		vaccel_error("Could not create shared object resource: %s",
			     strerror(ret));
		exit(1);
	}

	if (vaccel_session_init(&virtio_sess, VACCEL_PLUGIN_REMOTE)) {
		vaccel_error("Could not create virtio session");
		exit(1);
	}

	printf("Initialized virtio session with id: %" PRId64 "\n",
	       virtio_sess.id);

	if (vaccel_session_init(&local_sess, 0)) {
		vaccel_error("Could not create local session");
		exit(1);
	}

	printf("Initialized local session with id: %" PRId64 "\n",
	       local_sess.id);

	if (vaccel_resource_register(&object, &local_sess)) {
		vaccel_error("Could register shared object to local session");
		exit(1);
	}

	if (vaccel_resource_register(&object, &virtio_sess)) {
		vaccel_error("Could register shared object to virtio session");
		exit(1);
	}

	input = INPUT_VAL; /* some random input value */
	struct vaccel_arg read[1] = {
		{ .size = sizeof(input), .buf = &input, .argtype = ARGTYPE }
	};
	struct vaccel_arg write[1] = {
		{ .size = sizeof(output1), .buf = &output1, .argtype = ARGTYPE },
	};

	for (int i = 0; i < atoi(argv[2]); ++i) {
		sess = i % 2 ? &local_sess : &virtio_sess;

		ret = vaccel_exec_with_resource(sess, &object, "mytestfunc",
						read, 1, write, 1);
		if (ret) {
			vaccel_error("Could not run op: %d", ret);
			break;
		}
		printf("output1(2x%d): %d\n", input, output1);
		output1 = -1;
	}

	if (vaccel_resource_unregister(&object, &local_sess)) {
		vaccel_error("Could not unregister object from local session");
		exit(1);
	}

	if (vaccel_resource_unregister(&object, &virtio_sess)) {
		vaccel_error("Could not unregister object from virtio session");
		exit(1);
	}

	if (vaccel_resource_release(&object)) {
		vaccel_error("Could not destroy resource");
		exit(1);
	}

	if (vaccel_session_release(&local_sess)) {
		vaccel_error("Could not close session");
		exit(1);
	}

	if (vaccel_session_release(&virtio_sess)) {
		vaccel_error("Could not close session");
		exit(1);
	}

	return ret;
}
