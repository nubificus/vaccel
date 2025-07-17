// SPDX-License-Identifier: Apache-2.0

#define _POSIX_C_SOURCE 200809L

#include "error.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int vaccel_str_to_lower(const char *str, char *lower, size_t size,
			char **alloc_lower)
{
	if (!str || (!alloc_lower && (!lower || !size)))
		return VACCEL_EINVAL;

	char *buf;
	size_t len = strlen(str);
	if (alloc_lower == NULL) {
		buf = lower;
		if (len >= size)
			len = size - 1;
	} else {
		*alloc_lower = strdup(str);
		if (*alloc_lower == NULL)
			return VACCEL_EINVAL;

		buf = *alloc_lower;
	}

	for (size_t i = 0; i < len; i++) {
		if (str[i] == '\0')
			break;
		buf[i] = (char)tolower((unsigned char)str[i]);
	}
	buf[len] = '\0';

	return VACCEL_OK;
}
