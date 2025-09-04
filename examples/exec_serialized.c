// SPDX-License-Identifier: Apache-2.0

#include "common/mydata.h"
#include "vaccel.h"
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { MYDATA_COUNT = 6 };

int main(int argc, char *argv[])
{
	int ret;
	struct vaccel_session sess;
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
	sess.hint = VACCEL_PLUGIN_DEBUG;
	ret = vaccel_session_init(&sess, sess.hint);
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

	struct vaccel_arg_array read_args;
	ret = vaccel_arg_array_init(&read_args, 1);
	if (ret) {
		fprintf(stderr, "Could not initialize read args array\n");
		goto unregister_resource;
	}

	struct vaccel_arg_array write_args;
	ret = vaccel_arg_array_init(&write_args, 1);
	if (ret) {
		fprintf(stderr, "Could not initialize write args array\n");
		goto release_read_args_array;
	}

	struct mydata input_data;
	input_data.count = MYDATA_COUNT;
	input_data.array = malloc(input_data.count * sizeof(uint32_t));
	if (!input_data.array) {
		ret = VACCEL_ENOMEM;
		goto release_write_args_array;
	}

	printf("Input: ");
	for (uint32_t i = 0; i < input_data.count; i++) {
		input_data.array[i] = 2 * i;
		printf("%" PRId32 " ", input_data.array[i]);
	}
	printf("\n");

	ret = vaccel_arg_array_add_serialized(&read_args, VACCEL_ARG_CUSTOM,
					      MYDATA_TYPE_ID, &input_data,
					      sizeof(input_data),
					      mydata_serialize);
	if (ret) {
		fprintf(stderr, "Failed to pack input arg\n");
		goto free_input_data;
	}

	ret = vaccel_arg_array_add_serialized(&write_args, VACCEL_ARG_CUSTOM,
					      MYDATA_TYPE_ID, &input_data,
					      sizeof(input_data),
					      mydata_serialize);
	if (ret) {
		fprintf(stderr, "Failed to pack output arg\n");
		goto free_input_data;
	}

	struct mydata output_data;
	const int iter = (argc > 2) ? atoi(argv[2]) : 1;
	for (int i = 0; i < iter; i++) {
		vaccel_prof_region_start(&mytestfunc_stats);

		ret = vaccel_exec_with_resource(
			&sess, &object, "mytestfunc_nonser", read_args.args,
			read_args.count, write_args.args, write_args.count);

		vaccel_prof_region_stop(&mytestfunc_stats);

		if (ret) {
			fprintf(stderr, "Could not run op: %d\n", ret);
			goto free_input_data;
		}

		ret = vaccel_arg_array_get_serialized(
			&write_args, VACCEL_ARG_CUSTOM, MYDATA_TYPE_ID,
			&output_data, sizeof(output_data), mydata_deserialize);
		if (ret) {
			fprintf(stderr, "Failed to unpack output arg\n");
			goto free_input_data;
		}

		printf("Output: ");
		for (uint32_t i = 0; i < output_data.count; i++) {
			printf("%" PRId32 " ", output_data.array[i]);
		}
		printf("\n");

		free(output_data.array);
	}

free_input_data:
	free(input_data.array);
release_write_args_array:
	if (vaccel_arg_array_release(&write_args))
		fprintf(stderr, "Could not release write args array\n");
release_read_args_array:
	if (vaccel_arg_array_release(&read_args))
		fprintf(stderr, "Could not release read args array\n");
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
