// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "vaccel.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct mydata {
	uint32_t size;
	uint32_t *array;
};

enum { MYDATA_TYPE_ID = 100 };

/* Serializer function for `struct mydata` */
int mydata_serialize(const void *data, size_t data_size, uint32_t custom_id,
		     void **buf, size_t *size);

/* Deserializer function for `struct mydata` */
int mydata_deserialize(const void *buf, size_t size, uint32_t custom_id,
		       size_t data_size, void *out_data);
