// SPDX-License-Identifier: Apache-2.0

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "../src/utils.h"
#include <vaccel.h>

int main(int argc, char *argv[])
{
	if (argc != 3) {
		fprintf(stderr, "usage: %s path_shared_object iterations\n",
			argv[0]);
		return 0;
	}

	int input;
	int output1 = 0;
	int output2 = 0;

	struct vaccel_shared_object object;

	int ret = vaccel_shared_object_new(&object, argv[1]);
	if (ret) {
		fprintf(stderr, "Could not create shared object resource: %s",
			strerror(ret));
		exit(1);
	}

	struct vaccel_session sess;
	ret = vaccel_sess_init(&sess, 0);
	if (ret) {
		fprintf(stderr, "Could not create new shared object\n");
		exit(1);
	}
	printf("Initialized session with id: %u\n", sess.session_id);
	ret = vaccel_sess_register(&sess, object.resource);
	if (ret) {
		fprintf(stderr, "Could register shared object to session\n");
		exit(1);
	}

	struct vaccel_shared_object object2;
	size_t len;
	unsigned char *buff;
	ret = read_file(argv[1], (void **)&buff, &len);
	if (ret) {
		fprintf(stderr, "Could not read shared object file\n");
		exit(1);
	}

	ret = vaccel_shared_object_new_from_buffer(&object2, buff, len);
	if (ret) {
		fprintf(stderr,
			"Could not create shared object2 resource from buffer: %s\n",
			strerror(ret));
		exit(1);
	}

	ret = vaccel_sess_register(&sess, object2.resource);
	if (ret) {
		fprintf(stderr, "Could not register object 2 to session\n");
		exit(1);
	}

	input = 10; /* some random input value */
	struct vaccel_arg read[1] = {
		{ .size = sizeof(input), .buf = &input, .argtype = 42 }
	};
	struct vaccel_arg write[1] = {
		{ .size = sizeof(output1), .buf = &output1, .argtype = 42 },
	};

	for (int i = 0; i < atoi(argv[2]); ++i) {
		ret = vaccel_exec_with_resource(&sess, &object, "mytestfunc",
						read, 1, write, 1);
		if (ret) {
			fprintf(stderr, "Could not run op: %d\n", ret);
			goto close_session;
		}
	}
	printf("output1(2x%d): %d\n", input, output1);

	struct vaccel_arg read_2[1] = { { .size = sizeof(input),
					  .buf = &input } };
	struct vaccel_arg write_2[1] = {
		{ .size = sizeof(output2), .buf = &output2 },
	};

	for (int i = 0; i < atoi(argv[2]); ++i) {
		ret = vaccel_exec_with_resource(&sess, &object2, "mytestfunc",
						read_2, 1, write_2, 1);
		if (ret) {
			fprintf(stderr, "Could not run op: %d\n", ret);
			goto close_session;
		}
	}
	printf("output1(2x%d): %d\n", input, output2);

	ret = vaccel_sess_unregister(&sess, object.resource);
	if (ret) {
		fprintf(stderr, "Could not unregister object from session\n");
		exit(1);
	}

	ret = vaccel_sess_unregister(&sess, object2.resource);
	if (ret) {
		fprintf(stderr, "Could not unregister object 2 from session\n");
		exit(1);
	}

	ret = vaccel_shared_object_destroy(&object);
	if (ret) {
		fprintf(stderr, "Could not destroy object\n");
		exit(1);
	}

	ret = vaccel_shared_object_destroy(&object2);
	if (ret) {
		fprintf(stderr, "Could not destroy object2\n");
		exit(1);
	}

	ret = vaccel_sess_free(&sess);
	if (ret) {
		fprintf(stderr, "Could not close session\n");
		exit(1);
	}

	free(buff);

	return 0;

close_session:
	ret = vaccel_sess_unregister(&sess, object.resource);
	if (ret) {
		fprintf(stderr, "Could not unregister object from session\n");
		exit(1);
	}

	ret = vaccel_sess_unregister(&sess, object2.resource);
	if (ret) {
		fprintf(stderr, "Could not unregister object 2 from session\n");
		exit(1);
	}

	ret = vaccel_shared_object_destroy(&object);
	if (ret) {
		fprintf(stderr, "Could not destroy object\n");
		exit(1);
	}

	ret = vaccel_shared_object_destroy(&object2);
	if (ret) {
		fprintf(stderr, "Could not destroy object2\n");
		exit(1);
	}

	if (vaccel_sess_free(&sess) != VACCEL_OK) {
		fprintf(stderr, "Could not clear session\n");
		return 1;
	}

	free(buff);
	return ret;
}
