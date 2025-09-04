// SPDX-License-Identifier: Apache-2.0

#include "utils/fs.h"
#include "vaccel.h"
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { INPUT_VAL = 10 };

int main(int argc, char *argv[])
{
	int ret;
	struct vaccel_prof_region mytestfunc_stats =
		VACCEL_PROF_REGION_INIT("mytestfunc");

	int input = INPUT_VAL;
	int32_t output1 = 0;
	int32_t output2 = 0;

	if (argc < 2 || argc > 3) {
		fprintf(stderr, "Usage: %s <lib_file> [iterations]\n", argv[0]);
		return VACCEL_EINVAL;
	}

	struct vaccel_resource res1;
	ret = vaccel_resource_init(&res1, argv[1], VACCEL_RESOURCE_LIB);
	if (ret) {
		fprintf(stderr, "Could not initialize lib resource: %s\n",
			strerror(ret));
		return ret;
	}

	struct vaccel_session sess;
	ret = vaccel_session_init(&sess, 0);
	if (ret) {
		fprintf(stderr, "Could not initialize session\n");
		goto release_resource1;
	}
	printf("Initialized session with id: %" PRId64 "\n", sess.id);

	ret = vaccel_resource_register(&res1, &sess);
	if (ret) {
		fprintf(stderr, "Could not register lib with session\n");
		goto release_session;
	}

	struct vaccel_resource res2;
	size_t len;
	unsigned char *buff;
	ret = fs_file_read(argv[1], (void **)&buff, &len);
	if (ret) {
		fprintf(stderr, "Could not read lib file\n");
		goto unregister_resource1;
	}

	ret = vaccel_resource_init_from_buf(
		&res2, buff, len, VACCEL_RESOURCE_LIB, "lib.so", false);
	if (ret) {
		fprintf(stderr,
			"Could not initialize lib resource 2 from buffer: %s\n",
			strerror(ret));
		goto free_buff;
	}

	ret = vaccel_resource_register(&res2, &sess);
	if (ret) {
		fprintf(stderr, "Could not register lib 2 with session\n");
		goto release_resource2;
	}

	struct vaccel_arg_array read_args;
	ret = vaccel_arg_array_init(&read_args, 1);
	if (ret) {
		fprintf(stderr, "Could not initialize read args array\n");
		goto unregister_resource2;
	}

	struct vaccel_arg_array write_args;
	ret = vaccel_arg_array_init(&write_args, 1);
	if (ret) {
		fprintf(stderr, "Could not initialize write args array\n");
		goto release_read_args_array;
	}

	ret = vaccel_arg_array_add_int32(&read_args, &input);
	if (ret) {
		fprintf(stderr, "Failed to pack input arg\n");
		goto release_write_args_array;
	}

	ret = vaccel_arg_array_add_int32(&write_args, &output1);
	if (ret) {
		fprintf(stderr, "Failed to pack output1 arg\n");
		goto release_write_args_array;
	}

	const int iter = (argc > 2) ? atoi(argv[2]) : 1;
	for (int i = 0; i < iter; i++) {
		vaccel_prof_region_start(&mytestfunc_stats);

		ret = vaccel_exec_with_resource(&sess, &res1, "mytestfunc",
						read_args.args, read_args.count,
						write_args.args,
						write_args.count);

		vaccel_prof_region_stop(&mytestfunc_stats);

		if (ret) {
			fprintf(stderr, "Could not run op: %d\n", ret);
			goto release_write_args_array;
		}

		printf("output1: %" PRId32 "\n", output1);
	}

	vaccel_arg_array_clear(&write_args);

	ret = vaccel_arg_array_add_int32(&write_args, &output2);
	if (ret) {
		fprintf(stderr, "Failed to pack output2 arg\n");
		goto release_write_args_array;
	}

	for (int i = 0; i < iter; i++) {
		vaccel_prof_region_start(&mytestfunc_stats);

		ret = vaccel_exec_with_resource(&sess, &res2, "mytestfunc",
						read_args.args, read_args.count,
						write_args.args,
						write_args.count);

		vaccel_prof_region_stop(&mytestfunc_stats);

		if (ret) {
			fprintf(stderr, "Could not run op: %d\n", ret);
			goto release_write_args_array;
		}

		printf("output2: %" PRId32 "\n", output2);
	}

release_write_args_array:
	if (vaccel_arg_array_release(&write_args))
		fprintf(stderr, "Could not release write args array\n");
release_read_args_array:
	if (vaccel_arg_array_release(&read_args))
		fprintf(stderr, "Could not release read args array\n");
unregister_resource2:
	if (vaccel_resource_unregister(&res2, &sess))
		fprintf(stderr, "Could not unregister lib 2 from session\n");
release_resource2:
	if (vaccel_resource_release(&res2))
		fprintf(stderr, "Could not release lib resource 2\n");
free_buff:
	free(buff);
unregister_resource1:
	if (vaccel_resource_unregister(&res1, &sess))
		fprintf(stderr, "Could not unregister lib from session\n");
release_session:
	if (vaccel_session_release(&sess))
		fprintf(stderr, "Could not release session\n");
release_resource1:
	if (vaccel_resource_release(&res1))
		fprintf(stderr, "Could not release lib resource\n");

	vaccel_prof_region_print(&mytestfunc_stats);
	vaccel_prof_region_release(&mytestfunc_stats);

	return ret;
}
