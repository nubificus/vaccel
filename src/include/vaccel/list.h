// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define LIST_ENTRY_INIT(name) { &(name), &(name) }

struct vaccel_list_entry {
	/* pointer to the next list entry */
	struct vaccel_list_entry *next;

	/* pointer to the previous list entry */
	struct vaccel_list_entry *prev;
};

#ifdef __cplusplus
}
#endif
