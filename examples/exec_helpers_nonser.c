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

	*ser_buf = non_ser->size;

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

	struct mydata input_data;
	struct mydata *output_data = NULL;

	if (argc < 3) {
		vaccel_error("You must specify the number of iterations");
		return 1;
	}

	ret = vaccel_resource_init(&object, argv[1], VACCEL_RESOURCE_LIB);
	if (ret) {
		vaccel_error("Could not create shared object resource: %s",
			     strerror(ret));
		exit(1);
	}
	sess.hint = VACCEL_PLUGIN_DEBUG;
	ret = vaccel_session_init(&sess, sess.hint);
	if (ret != VACCEL_OK) {
		vaccel_error("Could not initialize session");
		return 1;
	}

	printf("Initialized session with id: %" PRId64 "\n", sess.id);

	ret = vaccel_resource_register(&object, &sess);
	if (ret) {
		vaccel_error("Could register shared object to session");
		exit(1);
	}

	struct vaccel_arg_list *read = vaccel_args_init(1);
	struct vaccel_arg_list *write = vaccel_args_init(1);

	if (!read || !write) {
		printf("Problem with creating arg-list\n");
		return 1;
	}

	input_data.size = ARR_LEN;
	input_data.array = malloc(ARR_LEN * sizeof(int));
	printf("Input: ");
	for (int i = 0; i < ARR_LEN; i++) {
		input_data.array[i] = 2 * i;
		printf("%d ", input_data.array[i]);
	}
	printf("\n");

	ret = vaccel_add_nonserial_arg(read, &input_data, 0, ser);
	if (ret != VACCEL_OK) {
		printf("Could not add non-serialized arg\n");
		return 1;
	}

	uint32_t expected_size = (input_data.size + 1) * sizeof(int);
	ret = vaccel_expect_nonserial_arg(write, expected_size);
	if (ret != VACCEL_OK) {
		printf("Could not define expected non-serialized arg\n");
		return 1;
	}

	for (int i = 0; i < atoi(argv[2]); ++i) {
		ret = vaccel_exec_with_resource(&sess, &object,
						"mytestfunc_nonser", read->list,
						read->size, write->list,
						write->size);

		if (ret) {
			vaccel_error("Could not run op: %d", ret);
			goto close_session;
		}
	}

	output_data = vaccel_extract_nonserial_arg(write->list, 0, deser);
	if (!output_data) {
		printf("Could not extract non-serialized arg\n");
		return 1;
	}

	printf("Output: ");
	uint32_t i;
	for (i = 0; i < output_data->size; i++) {
		printf("%d ", output_data->array[i]);
	}
	printf("\n");

close_session:

	/* Free non-serialised buffers */
	free(input_data.array);
	free(output_data->array);
	free(output_data);

	/* Delete arg-lists */
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
		exit(1);
	}

	if (vaccel_session_release(&sess) != VACCEL_OK) {
		vaccel_error("Could not clear session");
		return 1;
	}

	ret = vaccel_resource_release(&object);
	if (ret) {
		vaccel_error("Could not destroy object");
		exit(1);
	}

	return ret;
}
