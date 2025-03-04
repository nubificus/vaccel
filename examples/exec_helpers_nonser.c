// SPDX-License-Identifier: Apache-2.0

#include "vaccel.h"
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { ARR_LEN = 5 };

struct mydata {
	uint32_t size;
	int *array;
};

/* Serializer function for `struct mydata` */
void *ser(void *buf, uint32_t *bytes)
{
	struct mydata *non_ser = (struct mydata *)buf;

	uint32_t size = (non_ser->size + 1) * sizeof(int);
	int *ser_buf = malloc(size);

	*ser_buf = (int)non_ser->size;

	for (uint32_t i = 0; i < non_ser->size; i++)
		ser_buf[i + 1] = non_ser->array[i];

	*bytes = size;
	return ser_buf;
}

/* Deserializer function for `struct mydata` */
void *deser(void *buf, uint32_t __attribute__((unused)) bytes)
{
	int *ser_buf = (int *)buf;
	int size = ser_buf[0];

	if (!size)
		return NULL;

	struct mydata *new_buf = (struct mydata *)malloc(sizeof(struct mydata));

	new_buf->size = (uint32_t)size;
	new_buf->array = malloc(new_buf->size * sizeof(int));
	for (int i = 0; i < size; i++)
		new_buf->array[i] = ser_buf[i + 1];

	return new_buf;
}

int main(int argc, char *argv[])
{
	int ret;
	struct vaccel_session sess;
	struct vaccel_resource object;
	struct vaccel_prof_region mytestfunc_stats =
		VACCEL_PROF_REGION_INIT("mytestfunc");

	struct mydata input_data;
	struct mydata *output_data = NULL;

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

	struct vaccel_arg_list *read = vaccel_args_init(1);
	struct vaccel_arg_list *write = vaccel_args_init(1);

	if (!read || !write) {
		fprintf(stderr, "Problem with creating arg-list\n");
		ret = VACCEL_ENOMEM;
		goto unregister_resource;
	}

	input_data.size = ARR_LEN;
	input_data.array = malloc(ARR_LEN * sizeof(int));
	if (!input_data.array) {
		ret = VACCEL_ENOMEM;
		goto delete_args;
	}

	printf("Input: ");
	for (int i = 0; i < ARR_LEN; i++) {
		input_data.array[i] = 2 * i;
		printf("%d ", input_data.array[i]);
	}
	printf("\n");

	ret = vaccel_add_nonserial_arg(read, &input_data, 0, ser);
	if (ret) {
		printf("Could not add non-serialized arg\n");
		goto free_input_data;
	}

	uint32_t expected_size = (input_data.size + 1) * sizeof(int);
	ret = vaccel_expect_nonserial_arg(write, expected_size);
	if (ret) {
		printf("Could not define expected non-serialized arg\n");
		goto free_input_data;
	}

	const int iter = (argc > 2) ? atoi(argv[2]) : 1;
	for (int i = 0; i < iter; i++) {
		vaccel_prof_region_start(&mytestfunc_stats);

		ret = vaccel_exec_with_resource(&sess, &object,
						"mytestfunc_nonser", read->list,
						read->size, write->list,
						write->size);

		vaccel_prof_region_stop(&mytestfunc_stats);

		if (ret) {
			fprintf(stderr, "Could not run op: %d\n", ret);
			goto free_input_data;
		}

		output_data =
			vaccel_extract_nonserial_arg(write->list, 0, deser);
		if (!output_data) {
			printf("Could not extract non-serialized arg\n");
			goto free_input_data;
		}

		printf("Output: ");
		uint32_t i;
		for (i = 0; i < output_data->size; i++) {
			printf("%d ", output_data->array[i]);
		}
		printf("\n");

		free(output_data->array);
		free(output_data);
	}

free_input_data:
	free(input_data.array);
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
