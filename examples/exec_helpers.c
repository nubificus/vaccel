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
	int input_int;
	int output_int;
	struct vaccel_shared_object object;

	if (argc < 2) {
		fprintf(stderr, "You must specify the number of iterations\n");
		return 1;
	}

	ret = vaccel_shared_object_new(&object, argv[1]);
	if (ret) {
		fprintf(stderr, "Could not create shared object resource: %s",
			strerror(ret));
		exit(1);
	}
	sess.hint = VACCEL_PLUGIN_DEBUG;
	ret = vaccel_sess_init(&sess, sess.hint);
	if (ret != VACCEL_OK) {
		fprintf(stderr, "Could not initialize session\n");
		return 1;
	}

	printf("Initialized session with id: %u\n", sess.session_id);

	ret = vaccel_sess_register(&sess, object.resource);
	if (ret) {
		fprintf(stderr, "Could register shared object to session\n");
		exit(1);
	}

	printf("Initialized session with id: %u\n", sess.session_id);

	struct vaccel_arg_list *read = vaccel_args_init(1);
	struct vaccel_arg_list *write = vaccel_args_init(1);

	if (!read || !write) {
		printf("Problem with creating arg-list\n");
		return 1;
	}

	input_int = 15;
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
			fprintf(stderr, "Could not run op: %d\n", ret);
			goto close_session;
		}
	}

	int *outptr = vaccel_extract_serial_arg(write->list, 0);
	if (!outptr) {
		printf("Could not extract serialized arg\n");
		return 1;
	}

	printf("input     : %d\n", input_int);
	printf("output(2x): %d\n", *outptr);
	printf("output(2x): %d\n", output_int);

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

	ret = vaccel_sess_unregister(&sess, object.resource);
	if (ret) {
		fprintf(stderr, "Could not unregister object from session\n");
		exit(1);
	}

	if (vaccel_sess_free(&sess) != VACCEL_OK) {
		fprintf(stderr, "Could not clear session\n");
		return 1;
	}

	ret = vaccel_shared_object_destroy(&object);
	if (ret) {
		fprintf(stderr, "Could not destroy object\n");
		exit(1);
	}

	return ret;
}
