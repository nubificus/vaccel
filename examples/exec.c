// SPDX-License-Identifier: Apache-2.0

#include "vaccel.h"
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

enum { INPUT_VAL = 10 };

int main(int argc, char *argv[])
{
	int ret;
	struct vaccel_session sess;
	int32_t input = INPUT_VAL;
	int32_t output = 0;
	struct vaccel_prof_region mytestfunc_stats =
		VACCEL_PROF_REGION_INIT("mytestfunc");

	if (argc < 2 || argc > 3) {
		fprintf(stderr, "Usage: %s <lib_file> [iterations]\n", argv[0]);
		return VACCEL_EINVAL;
	}

	sess.hint = VACCEL_PLUGIN_DEBUG;
	ret = vaccel_session_init(&sess, sess.hint);
	if (ret) {
		fprintf(stderr, "Could not initialize session\n");
		return ret;
	}

	printf("Initialized session with id: %" PRId64 "\n", sess.id);

	struct vaccel_arg_array read_args;
	ret = vaccel_arg_array_init(&read_args, 1);
	if (ret) {
		fprintf(stderr, "Could not initialize read args array\n");
		goto release_session;
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

	ret = vaccel_arg_array_add_int32(&write_args, &output);
	if (ret) {
		fprintf(stderr, "Failed to pack output arg\n");
		goto release_write_args_array;
	}

	const int iter = (argc > 2) ? atoi(argv[2]) : 1;
	for (int i = 0; i < iter; i++) {
		vaccel_prof_region_start(&mytestfunc_stats);

		ret = vaccel_exec(&sess, argv[1], "mytestfunc", read_args.args,
				  read_args.count, write_args.args,
				  write_args.count);

		vaccel_prof_region_stop(&mytestfunc_stats);

		if (ret) {
			fprintf(stderr, "Could not run op: %d\n", ret);
			goto release_write_args_array;
		}

		printf("output: %" PRId32 "\n", output);
	}

release_write_args_array:
	if (vaccel_arg_array_release(&write_args))
		fprintf(stderr, "Could not release write args array\n");
release_read_args_array:
	if (vaccel_arg_array_release(&read_args))
		fprintf(stderr, "Could not release read args array\n");
release_session:
	if (vaccel_session_release(&sess))
		fprintf(stderr, "Could not release session\n");

	vaccel_prof_region_print(&mytestfunc_stats);
	vaccel_prof_region_release(&mytestfunc_stats);

	return ret;
}
