// SPDX-License-Identifier: Apache-2.0

#include "vaccel_args.h"
#include "vaccel.h"
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { EXAMPLE_INT = 15 };

int main(int argc, char *argv[])
{
	int ret;
	struct vaccel_session sess;
	int input_int;
	int output_int;
	struct vaccel_resource object;

	if (argc < 3) {
		vaccel_error(
			"You must specify path to shared object and the number of iterations");
		return 1;
	}

	ret = vaccel_resource_init(&object, argv[1], VACCEL_RESOURCE_LIB);
	if (ret) {
		vaccel_error("Could not create shared object resource: %s",
			     strerror(ret));
		return 1;
	}
	ret = vaccel_session_init(&sess, VACCEL_PLUGIN_DEBUG);

	if (ret != VACCEL_OK) {
		vaccel_error("Could not initialize session");
		return 1;
	}
	printf("Initialized session with id: %" PRId64 "\n", sess.id);
	ret = vaccel_resource_register(&object, &sess);
	if (ret) {
		vaccel_error("Could register shared object to session");
		return 1;
	}

	printf("Registered resource %" PRId64 " to session %" PRId64 "\n",
	       object.id, sess.id);

	struct vaccel_arg_list *read = vaccel_args_init(1);
	struct vaccel_arg_list *write = vaccel_args_init(1);

	if (!read || !write) {
		printf("Problem with creating arg-list\n");
		return 1;
	}

	input_int = EXAMPLE_INT;
	ret = vaccel_add_serial_arg(read, &input_int, sizeof(input_int));
	if (ret != VACCEL_OK) {
		printf("Could not add serialized arg\n");
		return 1;
	}

	ret = vaccel_expect_serial_arg(write, &output_int, sizeof(output_int));
	if (ret != VACCEL_OK) {
		printf("Could not define expected serialized arg\n");
		return 1;
	}

	for (int i = 0; i < atoi(argv[2]); ++i) {
		ret = vaccel_exec_with_resource(&sess, &object, "mytestfunc",
						read->list, read->size,
						write->list, write->size);

		if (ret) {
			vaccel_error("Could not run op: %d", ret);
			goto close_session;
		}
	}

	int *outptr = vaccel_extract_serial_arg(write->list, 0);
	if (!outptr) {
		printf("Could not extract serialized arg\n");
		return 1;
	}

	printf("input:                                  %d\n", input_int);
	printf("output(from vaccel_extract_serial_arg): %d\n", *outptr);
	printf("output(from buf):                       %d\n", output_int);

close_session:

	ret = vaccel_delete_args(read);
	if (ret != VACCEL_OK) {
		printf("Could not delete arg list\n");
		return 1;
	}

	ret = vaccel_delete_args(write);
	if (ret != VACCEL_OK) {
		printf("Could not delete arg list\n");
		return 1;
	}

	ret = vaccel_resource_unregister(&object, &sess);
	if (ret) {
		vaccel_error("Could not unregister object from session");
		return 1;
	}

	if (vaccel_session_release(&sess) != VACCEL_OK) {
		vaccel_error("Could not clear session");
		return 1;
	}

	ret = vaccel_resource_release(&object);
	if (ret) {
		vaccel_error("Could not destroy object");
		return 1;
	}

	return ret;
}
