// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <stdbool.h>

/* Check if a URL is valid */
bool net_path_is_url(const char *path);

/* Check if a URL exists */
bool net_path_exists(const char *path);

/* Download a file from a URL */
int net_file_download(const char *path, const char *download_path);
