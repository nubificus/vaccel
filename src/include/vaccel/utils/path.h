// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "enum.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Define vaccel_path_type_t, vaccel_path_type_to_str() and
 * vaccel_path_type_to_base_str() */
#define _ENUM_PREFIX VACCEL_PATH
#define VACCEL_PATH_TYPE_ENUM_LIST(VACCEL_ENUM_ITEM)  \
	VACCEL_ENUM_ITEM(LOCAL_FILE, 0, _ENUM_PREFIX) \
	VACCEL_ENUM_ITEM(LOCAL_DIR, _ENUM_PREFIX)     \
	VACCEL_ENUM_ITEM(REMOTE_FILE, _ENUM_PREFIX)

VACCEL_ENUM_DEF_WITH_STR_FUNCS(vaccel_path_type, _ENUM_PREFIX,
			       VACCEL_PATH_TYPE_ENUM_LIST)
#undef _ENUM_PREFIX

#ifdef __cplusplus
}
#endif
