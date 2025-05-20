// SPDX-License-Identifier: Apache-2.0

#include "vaccel.h"
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { INPUT_VAL = 15 };

int main(int argc, char *argv[])
{
	int ret;
	struct vaccel_session sess;
	int input = INPUT_VAL;
	int output = 0;
	struct vaccel_resource object;
	struct vaccel_prof_region mytestfunc_stats =
		VACCEL_PROF_REGION_INIT("mytestfunc");

	if (argc < 2 || argc > 3) {
		fprintf(stderr, "Usage: %s <lib_file> [iterations]\n", argv[0]);
		return VACCEL_EINVAL;
	}

	ret = vaccel_resource_init(&object, argv[1], VACCEL_RESOURCE_LIB);
	if (ret) {
		fprintf(stderr, "Could not initialize lib resource: %s\n",
			strerror(ret));
		return ret;
	}

	ret = vaccel_session_init(&sess, VACCEL_PLUGIN_DEBUG);
	if (ret) {
		fprintf(stderr, "Could not initialize session\n");
		goto release_resource;
	}

	printf("Initialized session with id: %" PRId64 "\n", sess.id);
	ret = vaccel_resource_register(&object, &sess);
	if (ret) {
		fprintf(stderr, "Could not register lib with session\n");
		goto release_session;
	}

	printf("Registered resource %" PRId64 " with session %" PRId64 "\n",
	       object.id, sess.id);

	struct vaccel_arg_list *read = vaccel_args_init(1);
	struct vaccel_arg_list *write = vaccel_args_init(1);

	if (!read || !write) {
		printf("Problem with creating arg-list\n");
		ret = VACCEL_ENOMEM;
		goto unregister_resource;
	}

	ret = vaccel_add_serial_arg(read, &input, sizeof(input));
	if (ret) {
		printf("Could not add serialized arg\n");
		goto delete_args;
	}

	ret = vaccel_expect_serial_arg(write, &output, sizeof(output));
	if (ret) {
		printf("Could not define expected serialized arg\n");
		goto delete_args;
	}

	const int iter = (argc > 2) ? atoi(argv[2]) : 1;
	for (int i = 0; i < iter; i++) {
		vaccel_prof_region_start(&mytestfunc_stats);

		ret = vaccel_exec_with_resource(&sess, &object, "mytestfunc",
						read->list, read->size,
						write->list, write->size);

		vaccel_prof_region_stop(&mytestfunc_stats);

		if (ret) {
			fprintf(stderr, "Could not run op: %d\n", ret);
			goto delete_args;
		}

		int *outptr = vaccel_extract_serial_arg(write->list, 0);
		if (!outptr) {
			printf("Could not extract serialized arg\n");
			goto delete_args;
		}

		printf("input:                                  %d\n", input);
		printf("output(from vaccel_extract_serial_arg): %d\n", *outptr);
		printf("output(from buf):                       %d\n", output);
	}

delete_args:
	if (vaccel_delete_args(read))
		fprintf(stderr, "Could not delete arg list\n");
	if (vaccel_delete_args(write))
		fprintf(stderr, "Could not delete arg list\n");
unregister_resource:
	if (vaccel_resource_unregister(&object, &sess))
		fprintf(stderr, "Could not unregister lib from session\n");
release_session:
	if (vaccel_session_release(&sess))
		fprintf(stderr, "Could not release session\n");
release_resource:
	if (vaccel_resource_release(&object))
		fprintf(stderr, "Could not release lib resource\n");

	vaccel_prof_region_print(&mytestfunc_stats);
	vaccel_prof_region_release(&mytestfunc_stats);

	return ret;
}
