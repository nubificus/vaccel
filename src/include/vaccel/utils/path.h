// SPDX-License-Identifier: Apache-2.0

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	VACCEL_PATH_LOCAL_FILE = 0,
	VACCEL_PATH_LOCAL_DIR,
	VACCEL_PATH_REMOTE_FILE,
	VACCEL_PATH_MAX
} vaccel_path_t;

#ifdef __cplusplus
}
#endif
