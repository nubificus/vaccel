// SPDX-License-Identifier: Apache-2.0

#include "common/mydata.h"
#include "vaccel.h"
#include <assert.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* We know we're getting only one read and only one write argument */

/* Test function for plain data */
int mytestfunc(struct vaccel_arg *input, size_t nr_in,
	       struct vaccel_arg *output, size_t nr_out)
{
	if (nr_in != 1 || nr_out != 1) {
		fprintf(stderr, "Invalid number of arguments\n");
		return VACCEL_EINVAL;
	}

	printf("I got nr_in: %zu, nr_out: %zu\n", nr_in, nr_out);

	struct vaccel_arg_array input_args;
	int ret = vaccel_arg_array_wrap(&input_args, input, nr_in);
	if (ret) {
		fprintf(stderr, "Failed to parse input args\n");
		return ret;
	}

	struct vaccel_arg_array output_args;
	ret = vaccel_arg_array_wrap(&output_args, output, nr_out);
	if (ret) {
		fprintf(stderr, "Failed to parse output args\n");
		return ret;
	}

	int32_t input_int;
	ret = vaccel_arg_array_get_int32(&input_args, &input_int);
	if (ret) {
		fprintf(stderr, "Failed to unpack input\n");
		return ret;
	}

	printf("I got input: %" PRId32 "\n", input_int);

	int32_t output_int = 2 * input_int;
	ret = vaccel_arg_array_set_int32(&output_args, &output_int);
	if (ret) {
		fprintf(stderr, "Failed to pack output\n");
		return ret;
	}

	printf("Will return output: %" PRId32 "\n", output_int);
	return VACCEL_OK;
}

/* Test function for serialized data */
int mytestfunc_nonser(struct vaccel_arg *input, size_t nr_in,
		      struct vaccel_arg *output, size_t nr_out)
{
	if (nr_in != 1 || nr_out != 1) {
		fprintf(stderr, "Invalid number of arguments\n");
		return VACCEL_EINVAL;
	}

	printf("I got nr_in: %zu, nr_out: %zu\n", nr_in, nr_out);

	struct vaccel_arg_array input_args;
	int ret = vaccel_arg_array_wrap(&input_args, input, nr_in);
	if (ret) {
		fprintf(stderr, "Failed to parse input args\n");
		return ret;
	}

	struct vaccel_arg_array output_args;
	ret = vaccel_arg_array_wrap(&output_args, output, nr_out);
	if (ret) {
		fprintf(stderr, "Failed to parse output args\n");
		return ret;
	}

	struct mydata input_data;
	ret = vaccel_arg_array_get_serialized(&input_args, VACCEL_ARG_CUSTOM,
					      MYDATA_TYPE_ID, &input_data,
					      sizeof(input_data),
					      mydata_deserialize);
	if (ret) {
		fprintf(stderr, "Failed to unpack input arg\n");
		return ret;
	}

	/* Copy the numbers */
	uint32_t *numbuf = malloc(input_data.count * sizeof(uint32_t));
	if (!numbuf)
		return VACCEL_ENOMEM;
	memcpy(numbuf, input_data.array, input_data.count * sizeof(uint32_t));

	/* Reverse the numbers */
	for (uint32_t i = 0; i < input_data.count; i++) {
		input_data.array[i] = numbuf[input_data.count - i - 1];
	}

	/* Output */
	struct mydata *output_data = &input_data;
	ret = vaccel_arg_array_set_serialized(&output_args, VACCEL_ARG_CUSTOM,
					      MYDATA_TYPE_ID, output_data,
					      sizeof(*output_data),
					      mydata_serialize);
	if (ret)
		fprintf(stderr, "Failed to pack output arg\n");

	/* Free Memory */
	free(output_data->array);
	free(numbuf);

	return ret;
}
