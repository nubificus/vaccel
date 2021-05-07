#ifndef __VACCEL_LIST_H__
#define __VACCEL_LIST_H__

#include <stdlib.h>
#include <stdbool.h>
#include <stddef.h>

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

#endif /* __VACCEL_LIST_H__ */
