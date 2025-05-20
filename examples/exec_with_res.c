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
	int ret;
	struct vaccel_resource resource;
	struct vaccel_prof_region mytestfunc_stats =
		VACCEL_PROF_REGION_INIT("mytestfunc");

	int input = INPUT_VAL;
	int output1 = 0;
	int output2 = 0;

	if (argc < 2 || argc > 3) {
		fprintf(stderr, "Usage: %s <lib_file> [iterations]\n", argv[0]);
		return VACCEL_EINVAL;
	}

	ret = vaccel_resource_init(&resource, argv[1], VACCEL_RESOURCE_LIB);
	if (ret) {
		fprintf(stderr, "Could not initialize lib resource: %s\n",
			strerror(ret));
		return ret;
	}

	struct vaccel_session sess;
	ret = vaccel_session_init(&sess, 0);
	if (ret) {
		fprintf(stderr, "Could not initialize session\n");
		goto release_resource;
	}
	printf("Initialized session with id: %" PRId64 "\n", sess.id);

	ret = vaccel_resource_register(&resource, &sess);
	if (ret) {
		fprintf(stderr, "Could not register lib with session\n");
		goto release_session;
	}

	struct vaccel_resource resource2;
	size_t len;
	unsigned char *buff;
	ret = fs_file_read(argv[1], (void **)&buff, &len);
	if (ret) {
		fprintf(stderr, "Could not read lib file\n");
		goto unregister_resource;
	}

	ret = vaccel_resource_init_from_buf(
		&resource2, buff, len, VACCEL_RESOURCE_LIB, "lib.so", false);
	if (ret) {
		fprintf(stderr,
			"Could not initialize lib resource 2 from buffer: %s\n",
			strerror(ret));
		goto free_buff;
	}

	ret = vaccel_resource_register(&resource2, &sess);
	if (ret) {
		fprintf(stderr, "Could not register lib 2 with session\n");
		goto release_resource2;
	}

	struct vaccel_arg read[] = {
		{ .size = sizeof(input), .buf = &input, .argtype = ARGTYPE }
	};
	struct vaccel_arg write[] = {
		{ .size = sizeof(output1), .buf = &output1, .argtype = ARGTYPE },
	};

	const int iter = (argc > 2) ? atoi(argv[2]) : 1;
	for (int i = 0; i < iter; i++) {
		vaccel_prof_region_start(&mytestfunc_stats);

		ret = vaccel_exec_with_resource(
			&sess, &resource, "mytestfunc", read,
			sizeof(read) / sizeof(read[0]), write,
			sizeof(write) / sizeof(write[0]));

		vaccel_prof_region_stop(&mytestfunc_stats);

		if (ret) {
			fprintf(stderr, "Could not run op: %d\n", ret);
			goto unregister_resource2;
		}

		printf("output1: %d\n", output1);
	}

	struct vaccel_arg read_2[] = {
		{ .size = sizeof(input), .buf = &input, .argtype = ARGTYPE }
	};
	struct vaccel_arg write_2[] = {
		{ .size = sizeof(output2), .buf = &output2, .argtype = ARGTYPE },
	};

	for (int i = 0; i < iter; i++) {
		vaccel_prof_region_start(&mytestfunc_stats);

		ret = vaccel_exec_with_resource(&sess, &resource2, "mytestfunc",
						read_2, 1, write_2, 1);

		vaccel_prof_region_stop(&mytestfunc_stats);

		if (ret) {
			fprintf(stderr, "Could not run op: %d\n", ret);
			goto unregister_resource2;
		}

		printf("output2: %d\n", output2);
	}

unregister_resource2:
	if (vaccel_resource_unregister(&resource2, &sess))
		fprintf(stderr, "Could not unregister lib 2 from session\n");
release_resource2:
	if (vaccel_resource_release(&resource2))
		fprintf(stderr, "Could not release lib resource 2\n");
free_buff:
	free(buff);
unregister_resource:
	if (vaccel_resource_unregister(&resource, &sess))
		fprintf(stderr, "Could not unregister lib from session\n");
release_session:
	if (vaccel_session_release(&sess))
		fprintf(stderr, "Could not release session\n");
release_resource:
	if (vaccel_resource_release(&resource))
		fprintf(stderr, "Could not release lib resource\n");

	vaccel_prof_region_print(&mytestfunc_stats);
	vaccel_prof_region_release(&mytestfunc_stats);

	return ret;
}
