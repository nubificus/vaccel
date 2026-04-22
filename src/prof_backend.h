// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "include/vaccel/prof_backend.h" // IWYU pragma: export

#ifdef __cplusplus
extern "C" {
#endif

/* Initialize the profiling selection with the built-in base backend. */
int prof_backends_bootstrap(void);

/* Clear the profiling backend selection. */
int prof_backends_cleanup(void);

/* Select active profiling backend after plugins have been loaded. */
int prof_backend_select(void);

/* Get the active profiling backend. */
const struct vaccel_prof_backend *vaccel_prof_backend_get(void);

/* Getter for the base backend. */
const struct vaccel_prof_backend *vaccel_prof_base_backend_get(void);

#ifdef __cplusplus
}
#endif
