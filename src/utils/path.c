// SPDX-License-Identifier: Apache-2.0

#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE

#include "error.h"
#include "log.h"
#include "path.h"
#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern bool net_path_is_url(const char *path);

bool path_is_url(const char *path)
{
	if (!path)
		return false;

	const char *http_prefix = "http://";
	const char *https_prefix = "https://";

	if (strncmp(path, http_prefix, strlen(http_prefix)) == 0 ||
	    strncmp(path, https_prefix, strlen(https_prefix)) == 0) {
		return net_path_is_url(path);
	}

	return false;
}

int path_init_from_parts(char *path, size_t size, const char *first_part, ...)
{
	if (!path || !size || !first_part)
		return VACCEL_EINVAL;

	if (size > PATH_MAX) {
		vaccel_error("Final path name too long for %s", first_part);
		return VACCEL_ENAMETOOLONG;
	}

	/* Verify the initial part fits within the buffer */
	strncpy(path, first_part, size - 1);
	path[size - 1] = '\0';
	if (strlen(path) >= size - 1) {
		vaccel_error("Initial path too long for %s", first_part);
		return VACCEL_ENAMETOOLONG;
	}

	va_list args;
	va_start(args, first_part);

	/* Build the path string */
	const char *part;
	while ((part = va_arg(args, const char *)) != NULL) {
		size_t cur_len = strlen(path);

		/* Check if there’s space for '/' and part */
		if (cur_len + 1 >= size - 1 ||
		    cur_len + strlen(part) >= size - 1) {
			vaccel_error("Path too long for %s", first_part);
			va_end(args);
			return VACCEL_ENAMETOOLONG;
		}

		/* Append '/' and next part */
		strncat(path, "/", size - cur_len - 1);
		strncat(path, part, size - cur_len - 1);
	}

	va_end(args);
	return 0;
}

int path_from_parts(char **path, const char *first_part, ...)
{
	if (!path || !first_part)
		return VACCEL_EINVAL;

	size_t total_len = strlen(first_part) + 1;
	const char *part = first_part;

	va_list args;
	va_start(args, first_part);

	/* Calculate the required length */
	while ((part = va_arg(args, const char *)) != NULL) {
		/* +1 for the '/' separator */
		total_len += strlen(part) + 1;
	}
	va_end(args);

	if (total_len > PATH_MAX) {
		vaccel_error("Final path name too long for %s", first_part);
		return VACCEL_ENAMETOOLONG;
	}

	/* Allocate memory for the final path */
	char *final_path = malloc(total_len);
	if (!final_path)
		return VACCEL_ENOMEM;

	/* Build the path string */
	final_path[0] = '\0';
	va_start(args, first_part);

	int ret = snprintf(final_path, total_len, "%s", first_part);
	if (ret < 0) {
		vaccel_error("Could not generate final path for %s: %s",
			     first_part, strerror(errno));
		va_end(args);
		free(final_path);
		return ret;
	}

	/* Loop again to append the remaining parts */
	while ((part = va_arg(args, const char *)) != NULL) {
		strncat(final_path, "/", total_len - strlen(final_path) - 1);
		strncat(final_path, part, total_len - strlen(final_path) - 1);
	}

	va_end(args);

	*path = final_path;

	return VACCEL_OK;
}

int path_file_name(char **name, const char *path)
{
	if (!name || !path)
		return VACCEL_EINVAL;

	/* Find the last occurrence of '/'.
	 * If there's no '/', the whole path is the filename */
	char *last_slash = strrchr(path, '/');
	const char *name_pos = last_slash ? (last_slash + 1) : path;

	size_t name_len = strlen(name_pos);
	if (name_len >= NAME_MAX) {
		vaccel_error("Filename %s too long", name_pos);
		return VACCEL_ENAMETOOLONG;
	}

	/* Copy and return filename */
	*name = strndup(name_pos, name_len);
	if (!*name)
		return VACCEL_ENOMEM;

	return VACCEL_OK;
}

int path_file_name_add_random_suffix(char *path, size_t *ext_len,
				     const char *base_path, const char *suffix)
{
	if (!path || !ext_len || !base_path || !suffix)
		return VACCEL_EINVAL;

	/* Extract filename */
	char *last_slash = strrchr(base_path, '/');
	const char *name = last_slash ? (last_slash + 1) : base_path;

	/* Extract extension */
	const char *ext = strrchr(name, '.');

	size_t base_len = ext ? (size_t)(ext - base_path) : strlen(base_path);
	*ext_len = ext ? strlen(ext) : 0;
	size_t path_len = base_len + strlen(suffix) + *ext_len + 1;

	/* Add path suffix before the extension (if any) */
	int ret = snprintf(path, path_len, "%.*s%s%s", (int)base_len, base_path,
			   suffix, ext ? ext : "");
	if (ret < 0) {
		vaccel_error("Could not insert random suffix for %s: %s",
			     base_path, strerror(errno));
		return ret;
	}
	if (ret >= PATH_MAX) {
		vaccel_error("Random suffix path too long for %s", base_path);
		return VACCEL_ENAMETOOLONG;
	}

	return VACCEL_OK;
}
