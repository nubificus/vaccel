// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

int vaccel_str_to_lower(char *str, size_t size, char **alloc_lower);

#ifdef __cplusplus
}
#endif
