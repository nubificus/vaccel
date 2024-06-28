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
