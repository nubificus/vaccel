// SPDX-License-Identifier: Apache-2.0

#include "utils/fs.h"
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
	int output1 = 0;
	int output2 = 0;

	struct vaccel_resource resource;

	int ret = vaccel_resource_init(&resource, argv[1], VACCEL_RESOURCE_LIB);
	if (ret) {
		vaccel_error("Could not create shared object resource: %s",
			     strerror(ret));
		exit(1);
	}

	struct vaccel_session sess;
	ret = vaccel_session_init(&sess, 0);
	if (ret) {
		vaccel_error("Could not create new shared object");
		exit(1);
	}
	printf("Initialized session with id: %" PRId64 "\n", sess.id);

	ret = vaccel_resource_register(&resource, &sess);
	if (ret) {
		vaccel_error("Could register shared object to session");
		exit(1);
	}

	struct vaccel_resource resource2;
	size_t len;
	unsigned char *buff;
	ret = fs_file_read(argv[1], (void **)&buff, &len);
	if (ret) {
		vaccel_error("Could not read shared object file");
		exit(1);
	}

	ret = vaccel_resource_init_from_buf(&resource2, buff, len,
					    VACCEL_RESOURCE_LIB, "lib.so");
	if (ret) {
		vaccel_error(
			"Could not create shared object2 resource from buffer: %s",
			strerror(ret));
		exit(1);
	}

	ret = vaccel_resource_register(&resource2, &sess);
	if (ret) {
		vaccel_error("Could not register object 2 to session");
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
		ret = vaccel_exec_with_resource(&sess, &resource, "mytestfunc",
						read, 1, write, 1);
		if (ret) {
			vaccel_error("Could not run op: %d", ret);
			goto close_session;
		}
	}
	printf("output1: %d\n", output1);

	struct vaccel_arg read_2[1] = { { .size = sizeof(input),
					  .buf = &input } };
	struct vaccel_arg write_2[1] = {
		{ .size = sizeof(output2), .buf = &output2 },
	};

	for (int i = 0; i < atoi(argv[2]); ++i) {
		ret = vaccel_exec_with_resource(&sess, &resource2, "mytestfunc",
						read_2, 1, write_2, 1);
		if (ret) {
			vaccel_error("Could not run op: %d", ret);
			goto close_session;
		}
	}
	printf("output2: %d\n", output2);

close_session:
	ret = vaccel_resource_unregister(&resource, &sess);
	if (ret) {
		vaccel_error("Could not unregister object from session");
		exit(1);
	}

	ret = vaccel_resource_unregister(&resource2, &sess);
	if (ret) {
		vaccel_error("Could not unregister object 2 from session");
		exit(1);
	}

	ret = vaccel_resource_release(&resource);
	if (ret) {
		vaccel_error("Could not destroy resource %" PRId64,
			     resource.id);
		exit(1);
	}

	ret = vaccel_resource_release(&resource2);
	if (ret) {
		vaccel_error("Could not destroy resource %" PRId64,
			     resource2.id);
		exit(1);
	}

	ret = vaccel_session_release(&sess);
	if (ret) {
		vaccel_error("Could not close session");
		exit(1);
	}

	free(buff);

	return 0;
}
