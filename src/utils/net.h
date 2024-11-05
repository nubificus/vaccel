// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Check if a URL is valid.
 * NOTE: If libcurl is not available returns true unless path is NULL */
bool net_path_is_url(const char *path);

/* Check if a URL exists
 * NOTE: If libcurl is not available returns false unless path is NULL */
bool net_path_exists(const char *path);

/* Download a file from a URL
 * NOTE: Only supported if libcurl is available */
int net_file_download(const char *path, const char *download_path);

#ifdef __cplusplus
}
#endif
