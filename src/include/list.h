// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

#define LIST_ENTRY_INIT(name) { &(name), &(name) }

typedef struct list_entry {
	struct list_entry *next;
	struct list_entry *prev;
} list_entry_t;

/* Our list type is actually just a normal entry type */
typedef list_entry_t list_t;

#ifdef __cplusplus
}
#endif
