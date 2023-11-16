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

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#include <vaccel.h>

struct mydata
{
	uint32_t size;
	int* array;
};

/* Serializer function for `struct mydata` */
void* ser(void* buf, uint32_t* bytes){
	struct mydata* non_ser = (struct mydata*)buf;

	uint32_t size = (non_ser->size + 1) * sizeof(int);
	int* ser_buf = malloc(size);

	memcpy(ser_buf, (int*)(&non_ser->size), sizeof(int));

	for(int i=0; i<(int)non_ser->size; i++)
		memcpy(&ser_buf[i+1], &non_ser->array[i], sizeof(int));


	*bytes = size;
	return ser_buf;
}

/* Deserializer function for `struct mydata` */
void* deser(void* buf, uint32_t __attribute__((unused)) bytes){
	int *ser_buf = (int*)buf;
	int size = ser_buf[0];

	struct mydata* new_buf =
		(struct mydata*)malloc(sizeof(struct mydata));

	new_buf->size = (uint32_t)size;
	new_buf->array = malloc(new_buf->size * sizeof(int));
	for(int i=0; i<size; i++)
		memcpy(&new_buf->array[i], &ser_buf[i+1], sizeof(int));

	return new_buf;
}

/* We know we're getting only one read and only one write argument */

/* Test function for serialized data */
int mytestfunc(struct vaccel_arg *input, size_t nr_in,
	       struct vaccel_arg *output, size_t nr_out)
{
	assert(nr_in >= 1);
	assert(nr_out >= 1);

	/* Input */
	int* input_int = vaccel_extract_serial_arg(input, 0);
	
	printf("I got nr_in: %ld, nr_out: %ld\n", nr_in, nr_out);
	printf("I got this input: %d\n", *input_int);

	/* Output */
	int output_int = 2*(*input_int);
	vaccel_write_serial_arg(output, 0, &output_int);
	return 0;
}

/* Test function for non-serialized data */
int mytestfunc_nonser(struct vaccel_arg *input, size_t nr_in,
	       struct vaccel_arg *output, size_t nr_out)
{
	assert(nr_in >= 1);
	assert(nr_out >= 1);
	printf("I got nr_in: %ld, nr_out: %ld\n", nr_in, nr_out);

	/* Input */
	struct mydata* input_data =
		vaccel_extract_nonserial_arg(input, 0, deser);

	if (!input_data) {
		printf("Could not extract non-serialized arg\n");
		return 1;
	}

	/* Copy the numbers */
	int *numbuf = malloc(input_data->size*sizeof(int));
	memcpy(numbuf, input_data->array, input_data->size * sizeof(int));
	
	/* Reverse the numbers */
	for(int i=0; i<(int)input_data->size; i++){
		input_data->array[i] = numbuf[input_data->size - i - 1];
	}

	/* Output */
	struct mydata* output_data = input_data;
	int ret = vaccel_write_nonserial_arg(output, 0, output_data, ser);
	
	if (ret != VACCEL_OK) {
		printf("Could not write to non-serialized arg\n");
		return 1;
	}

	/* Free Memory */
	free(output_data->array);
	free(output_data);
	free(numbuf);

	return 0;
}