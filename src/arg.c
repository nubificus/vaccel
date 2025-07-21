// SPDX-License-Identifier: Apache-2.0

#include "arg.h"
#include "error.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int vaccel_arg_init(struct vaccel_arg *arg, void *buf, uint32_t size,
		    uint32_t argtype)
{
	if (!arg)
		return VACCEL_EINVAL;

	arg->buf = buf;
	arg->size = size;
	arg->argtype = argtype;

	return VACCEL_OK;
}

int vaccel_arg_release(struct vaccel_arg *arg)
{
	if (!arg)
		return VACCEL_EINVAL;

	arg->buf = NULL;
	arg->size = 0;
	arg->argtype = 0;

	return VACCEL_OK;
}

int vaccel_arg_new(struct vaccel_arg **arg, void *buf, uint32_t size,
		   uint32_t argtype)
{
	if (!arg)
		return VACCEL_EINVAL;

	struct vaccel_arg *a =
		(struct vaccel_arg *)malloc(sizeof(struct vaccel_arg));
	if (!a)
		return VACCEL_ENOMEM;

	int ret = vaccel_arg_init(a, buf, size, argtype);
	if (ret) {
		free(a);
		return ret;
	}

	*arg = a;

	return VACCEL_OK;
}

int vaccel_arg_delete(struct vaccel_arg *arg)
{
	int ret = vaccel_arg_release(arg);
	if (ret)
		return ret;

	free(arg);

	return VACCEL_OK;
}

struct vaccel_arg_list *vaccel_args_init(uint32_t size)
{
	if (size == 0)
		return NULL;

	struct vaccel_arg_list *arg_list = (struct vaccel_arg_list *)malloc(
		sizeof(struct vaccel_arg_list));
	if (!arg_list)
		return NULL;

	arg_list->list =
		(struct vaccel_arg *)malloc(size * sizeof(struct vaccel_arg));
	if (!arg_list->list) {
		free(arg_list);
		return NULL;
	}

	arg_list->idcs_allocated_space = (int *)malloc(size * sizeof(int));
	if (!arg_list->idcs_allocated_space) {
		free(arg_list->list);
		free(arg_list);
		return NULL;
	}

	arg_list->size = size;
	arg_list->curr_idx = 0;

	return arg_list;
}

int vaccel_add_serial_arg(struct vaccel_arg_list *args, void *buf,
			  uint32_t size)
{
	if (!args || !buf || !size)
		return VACCEL_EINVAL;

	int curr_idx = args->curr_idx;

	if (curr_idx >= (int)args->size)
		return VACCEL_EINVAL;

	args->list[curr_idx].size = size;
	args->list[curr_idx].buf = buf;
	args->list[curr_idx].argtype = 0;

	/* The arg buffer is not allocated by us */
	args->idcs_allocated_space[curr_idx] = 0;

	args->curr_idx++;

	return VACCEL_OK;
}

int vaccel_add_nonserial_arg(struct vaccel_arg_list *args, void *buf,
			     uint32_t argtype,
			     void *(*serializer)(void *, uint32_t *))
{
	if (!args || !buf || !serializer)
		return VACCEL_EINVAL;

	int curr_idx = args->curr_idx;

	if (curr_idx >= (int)args->size)
		return VACCEL_EINVAL;

	uint32_t bytes;
	void *ser_buf = serializer(buf, &bytes);

	if (!ser_buf || !bytes)
		return VACCEL_EINVAL;

	args->list[curr_idx].buf = ser_buf;
	args->list[curr_idx].size = bytes;
	args->list[curr_idx].argtype = argtype;

	/* The arg buffer is allocated by us */
	args->idcs_allocated_space[curr_idx] = 1;

	args->curr_idx++;

	return VACCEL_OK;
}

int vaccel_expect_serial_arg(struct vaccel_arg_list *args, void *buf,
			     uint32_t size)
{
	if (!args || !buf || !size)
		return VACCEL_EINVAL;

	int curr_idx = args->curr_idx;

	if (curr_idx >= (int)args->size)
		return VACCEL_EINVAL;

	args->list[curr_idx].buf = buf;
	args->list[curr_idx].size = size;
	args->list[curr_idx].argtype = 0;

	/* The arg buffer is not allocated by us */
	args->idcs_allocated_space[curr_idx] = 0;

	args->curr_idx++;

	return VACCEL_OK;
}

int vaccel_expect_nonserial_arg(struct vaccel_arg_list *args,
				uint32_t expected_size)
{
	if (!args || !expected_size)
		return VACCEL_EINVAL;

	int curr_idx = args->curr_idx;

	if (curr_idx >= (int)args->size)
		return VACCEL_EINVAL;

	args->list[curr_idx].buf = calloc(1, expected_size);
	if (!args->list[curr_idx].buf)
		return VACCEL_ENOMEM;

	args->list[curr_idx].argtype = 0;
	args->list[curr_idx].size = expected_size;

	/* The arg buffer is allocated by us */
	args->idcs_allocated_space[curr_idx] = 1;

	args->curr_idx++;

	return VACCEL_OK;
}

void *vaccel_extract_serial_arg(struct vaccel_arg *args, int idx)
{
	if (!args || idx < 0)
		return NULL;

	return args[idx].buf;
}

void *vaccel_extract_nonserial_arg(struct vaccel_arg *args, int idx,
				   void *(*deserializer)(void *, uint32_t))
{
	if (idx < 0 || !args || !deserializer)
		return NULL;

	return deserializer(args[idx].buf, args[idx].size);
}

int vaccel_write_serial_arg(struct vaccel_arg *args, int idx, void *buf)
{
	if (idx < 0 || !buf || !args)
		return VACCEL_EINVAL;

	if (!memcpy(args[idx].buf, buf, args[idx].size))
		return VACCEL_ENOMEM;

	return VACCEL_OK;
}

int vaccel_write_nonserial_arg(struct vaccel_arg *args, int idx, void *buf,
			       void *(*serializer)(void *, uint32_t *))
{
	if (idx < 0 || !buf || !serializer || !args)
		return VACCEL_EINVAL;

	uint32_t bytes;
	void *ser_buf = serializer(buf, &bytes);

	if (!ser_buf || bytes <= 0)
		return VACCEL_EINVAL;

	void *dest = memcpy(args[idx].buf, ser_buf, bytes);

	free(ser_buf);

	if (!dest)
		return VACCEL_ENOMEM;

	return VACCEL_OK;
}

int vaccel_delete_args(struct vaccel_arg_list *args)
{
	if (!args)
		return VACCEL_EINVAL;

	if (!args->list || !args->size || !args->idcs_allocated_space)
		return VACCEL_EINVAL;

	uint32_t i;
	for (i = 0; i < args->size; i++) {
		if (args->idcs_allocated_space[i] == 1) {
			free(args->list[i].buf);
		}
	}

	free(args->list);
	free(args->idcs_allocated_space);
	free(args);

	return VACCEL_OK;
}
