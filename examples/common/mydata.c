// SPDX-License-Identifier: Apache-2.0

#include "mydata.h"
#include "vaccel.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

int mydata_serialize(const void *data, size_t data_size, uint32_t custom_id,
		     void **buf, size_t *size)
{
	if (!data || !buf || !size || !custom_id)
		return VACCEL_EINVAL;

	struct mydata *non_ser = (struct mydata *)data;
	if (data_size != sizeof(*non_ser))
		return VACCEL_EINVAL;

	size_t ser_size = ((size_t)non_ser->count + 1) * sizeof(uint32_t);
	uint32_t *ser_buf = malloc(ser_size);
	if (!ser_buf)
		return VACCEL_ENOMEM;

	/* If the struct is empty skip element serialization */
	if (non_ser->array && non_ser->count) {
		ser_buf[0] = (int)non_ser->count;
		for (uint32_t i = 0; i < non_ser->count; i++)
			ser_buf[i + 1] = non_ser->array[i];
	}

	*buf = ser_buf;
	*size = ser_size;
	return VACCEL_OK;
}

int mydata_deserialize(const void *buf, size_t size, uint32_t custom_id,
		       size_t data_size, void *out_data)
{
	(void)size;

	if (!buf || !out_data || !custom_id)
		return VACCEL_EINVAL;

	uint32_t *ser_buf = (uint32_t *)buf;
	struct mydata *deser_buf = (struct mydata *)out_data;

	if (data_size != sizeof(*deser_buf))
		return VACCEL_EINVAL;

	deser_buf->count = ser_buf[0];
	deser_buf->array = malloc(deser_buf->count * sizeof(uint32_t));
	if (!deser_buf->array)
		return VACCEL_ENOMEM;

	for (uint32_t i = 0; i < deser_buf->count; i++)
		deser_buf->array[i] = ser_buf[i + 1];

	return VACCEL_OK;
}
