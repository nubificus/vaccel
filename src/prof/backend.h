// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "include/vaccel/prof/backend.h" // IWYU pragma: export

/* Name of the built-in backend; reserved as a plugin name. */
#define VACCEL_PROF_BACKEND_BUILTIN_NAME "builtin"

#ifdef __cplusplus
extern "C" {
#endif

/* Initialize the profiling selection with the built-in backend. */
int prof_backends_bootstrap(void);

/* Clear the profiling backend selection. */
int prof_backends_cleanup(void);

/* Select active profiling backend after plugins have been loaded. */
int prof_backend_select(void);

/* Get the active profiling backend. */
const struct vaccel_prof_backend *vaccel_prof_backend_get(void);

/* Getter for the built-in backend. */
const struct vaccel_prof_backend *vaccel_prof_builtin_backend_get(void);

#ifdef __cplusplus
}
#endif
