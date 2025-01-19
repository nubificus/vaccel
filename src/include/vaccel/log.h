// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "utils/enum.h"
#include <slog.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Define vaccel_log_level_t, vaccel_log_level_to_str() and
 * vaccel_log_level_to_base_str() */
#define _ENUM_PREFIX VACCEL_LOG
#define VACCEL_LOG_LEVEL_ENUM_LIST(VACCEL_ENUM_ITEM) \
	VACCEL_ENUM_ITEM(ERROR, 1, _ENUM_PREFIX)     \
	VACCEL_ENUM_ITEM(WARN, _ENUM_PREFIX)         \
	VACCEL_ENUM_ITEM(INFO, _ENUM_PREFIX)         \
	VACCEL_ENUM_ITEM(DEBUG, _ENUM_PREFIX)

VACCEL_ENUM_DEF_WITH_STR_FUNCS(vaccel_log_level, _ENUM_PREFIX,
			       VACCEL_LOG_LEVEL_ENUM_LIST)
#undef _ENUM_PREFIX

#define vaccel_info slog_info
#define vaccel_warn slog_warn
#define vaccel_debug slog_debug
#define vaccel_error slog_error
#define vaccel_trace slog_trace
#define vaccel_fatal slog_fatal

int vaccel_log_init(void);
int vaccel_log_shutdown(void);

#ifdef __cplusplus
}
#endif
