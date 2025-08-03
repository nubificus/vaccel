// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "include/vaccel/session.h" // IWYU pragma: export
#include "list.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Initialize shared session objects */
int sessions_bootstrap(void);

/* Cleanup shared session objects */
int sessions_cleanup(void);

/* Helper macros for iterating lists of containers */
#define session_for_each_resource(iter, list)                                 \
	list_for_each_container((iter), (list), struct resource_registration, \
				session_entry)

#define session_for_each_resource_safe(iter, tmp, list)            \
	list_for_each_container_safe((iter), (tmp), (list),        \
				     struct resource_registration, \
				     session_entry)

#ifdef __cplusplus
}
#endif
