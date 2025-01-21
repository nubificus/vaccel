// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "config.h"

#ifdef __cplusplus
extern "C" {
#endif

int vaccel_bootstrap(void);
int vaccel_bootstrap_with_config(struct vaccel_config *config);
int vaccel_cleanup(void);
bool vaccel_is_initialized(void);
const char *vaccel_rundir(void);
const struct vaccel_config *vaccel_config(void);

#ifdef __cplusplus
}
#endif
