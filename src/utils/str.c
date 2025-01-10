// SPDX-License-Identifier: Apache-2.0

#define _POSIX_C_SOURCE 200809L

#include "error.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int vaccel_str_to_lower(char *str, size_t size, char **alloc_lower)
{
	if (!str || (!alloc_lower && !size))
		return VACCEL_EINVAL;

	char *lower;
	if (alloc_lower == NULL) {
		lower = str;
	} else {
		size = strlen(str);

		*alloc_lower = strdup(str);
		if (*alloc_lower == NULL)
			return VACCEL_ENOMEM;

		lower = *alloc_lower;
	}

	for (size_t i = 0; i < size; i++) {
		if (lower[i] == '\0')
			break;
		lower[i] = (char)tolower((unsigned char)lower[i]);
	}

	return VACCEL_OK;
}
