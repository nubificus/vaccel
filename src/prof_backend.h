// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "include/vaccel/prof_backend.h" // IWYU pragma: export
#include "prof.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Initialize the backend registry with the built-in base backend. */
int prof_backends_bootstrap(void);

/* Clear the backend registry. Does not touch entries' memory. */
int prof_backends_cleanup(void);

/* Resolve the active backend based on `vaccel_config()->profiling_backend`.
 * Always returns a valid pointer; falls back to base if the requested
 * backend is unknown, warning exactly once per unknown name. */
const struct vaccel_prof_backend *vaccel_prof_backend_get(void);

/* Getter for the always-present base backend. */
const struct vaccel_prof_backend *vaccel_prof_base_backend_get(void);

#ifdef __cplusplus
}
#endif
