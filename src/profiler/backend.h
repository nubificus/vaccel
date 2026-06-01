// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "include/vaccel/profiler/backend.h" // IWYU pragma: export

/* Name of the built-in backend; reserved as a plugin name. */
#define VACCEL_PROFILER_BACKEND_BUILTIN_NAME "builtin"

#ifdef __cplusplus
extern "C" {
#endif

/* Initialize the profiler selection with the built-in backend. */
int profiler_backends_bootstrap(void);

/* Clear the profiler backend selection. */
int profiler_backends_cleanup(void);

/* Select active profiler backend after plugins have been loaded. */
int profiler_backend_select(void);

/* Get the active profiler backend. */
const struct vaccel_profiler_backend *vaccel_profiler_backend_get(void);

/* Getter for the built-in backend. */
const struct vaccel_profiler_backend *vaccel_profiler_builtin_backend_get(void);

#ifdef __cplusplus
}
#endif
