/*
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <vaccel.h>

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

	memcpy(ser_buf, (int *)(&non_ser->size), sizeof(int));

	uint32_t i;
	for (i = 0; i < non_ser->size; i++)
		memcpy(&ser_buf[i + 1], &non_ser->array[i], sizeof(int));

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
		memcpy(&new_buf->array[i], &ser_buf[i + 1], sizeof(int));

	return new_buf;
}

int main(int argc, char *argv[])
{
	int ret;
	struct vaccel_session sess;
	struct vaccel_shared_object object;

	struct mydata input_data;
	struct mydata *output_data = NULL;

	if (argc < 3) {
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

	struct vaccel_arg_list *read = vaccel_args_init(1);
	struct vaccel_arg_list *write = vaccel_args_init(1);

	if (!read || !write) {
		printf("Problem with creating arg-list\n");
		return 1;
	}

	input_data.size = 5;
	input_data.array = malloc(5 * sizeof(int));
	printf("Input: ");
	for (int i = 0; i < 5; i++) {
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
			fprintf(stderr, "Could not run op: %d\n", ret);
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
