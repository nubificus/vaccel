// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <slog.h>

#ifdef __cplusplus
extern "C" {
#endif

int vaccel_log_init(void);
int vaccel_log_shutdown(void);

#define vaccel_info slog_info
#define vaccel_warn slog_warn
#define vaccel_debug slog_debug
#define vaccel_error slog_error
#define vaccel_trace slog_trace
#define vaccel_fatal slog_fatal

#ifdef __cplusplus
}
#endif
