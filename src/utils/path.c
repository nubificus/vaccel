// SPDX-License-Identifier: Apache-2.0

#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE

#include "error.h"
#include "fs.h"
#include "log.h"
#include "net.h"
#include "path.h"
#include <errno.h>
#include <limits.h>
#include <linux/limits.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool uri_is_remote(const char *uri)
{
	if (!uri)
		return false;

	const char *https_prefix = "https";
	const char *http_prefix = "http";
	const char *ftp_prefix = "ftp";

	if (strncmp(uri, https_prefix, strlen(https_prefix)) == 0 ||
	    strncmp(uri, http_prefix, strlen(http_prefix)) == 0 ||
	    strncmp(uri, ftp_prefix, strlen(ftp_prefix)) == 0) {
		return true;
	}

	return false;
}

static bool uri_is_local(const char *uri)
{
	if (!uri)
		return false;

	const char *local_prefix = "file";

	if (strncmp(uri, local_prefix, strlen(local_prefix)) == 0) {
		return true;
	}

	return false;
}

static int parse_uri(const char **path, vaccel_path_type_t *type,
		     const char *uri)
{
	if (!uri || (!path && !type))
		return VACCEL_EINVAL;

	const char *out_path = NULL;
	vaccel_path_type_t out_type = VACCEL_PATH_MAX;

	const char *separator = strstr(uri, "://");

	if (separator && uri_is_remote(uri)) {
		/* uri is remote */
		out_path = uri;
		if (net_path_is_url(out_path))
			out_type = VACCEL_PATH_REMOTE_FILE;
	} else if ((separator && uri_is_local(uri)) || !separator) {
		/* uri is local */
		if (separator) {
			out_path = separator + 3;
		} else {
			vaccel_warn(
				"Path does not seem to have a `<prefix>://`");
			vaccel_warn("Assuming %s is a local path", uri);
			out_path = uri;
		}
		if (fs_path_is_dir(out_path)) {
			out_type = VACCEL_PATH_LOCAL_DIR;
		} else if (fs_path_is_file(out_path)) {
			out_type = VACCEL_PATH_LOCAL_FILE;
		}
	}
	if (out_path == NULL) {
		vaccel_error("Unsupported URI prefix for %s", uri);
		return VACCEL_EINVAL;
	}
	if (out_type == VACCEL_PATH_MAX) {
		vaccel_error("Invalid path %s", out_path);
		return VACCEL_EINVAL;
	}
	if (strlen(out_path) > PATH_MAX) {
		vaccel_error("Path %s too long", out_path);
		return VACCEL_ENAMETOOLONG;
	}

	if (path != NULL)
		*path = out_path;
	if (type != NULL)
		*type = out_type;

	return VACCEL_OK;
}

int path_init_from_uri(char *path, size_t size, vaccel_path_type_t *type,
		       const char *uri)
{
	if (!uri || (path && !size) || (!path && !type))
		return VACCEL_EINVAL;

	const char *out_path;
	vaccel_path_type_t out_type;
	int ret = parse_uri(&out_path, &out_type, uri);
	if (ret)
		return ret;

	if (path != NULL) {
		if (strlen(out_path) >= size) {
			vaccel_error("Path %s does not fit into buf", out_path);
			return VACCEL_ENAMETOOLONG;
		}
		strncpy(path, out_path, size);
	}
	if (type != NULL)
		*type = out_type;

	return VACCEL_OK;
}

int path_from_uri(char **path, vaccel_path_type_t *type, const char *uri)
{
	if (!uri || (!path && !type))
		return VACCEL_EINVAL;

	const char *out_path;
	vaccel_path_type_t out_type;
	int ret = parse_uri(&out_path, &out_type, uri);
	if (ret)
		return ret;

	if (path != NULL) {
		*path = strdup(out_path);
		if (*path == NULL)
			return VACCEL_ENOMEM;
	}
	if (type != NULL)
		*type = out_type;

	return VACCEL_OK;
}

bool path_is_url(const char *path)
{
	if (!path)
		return false;

	vaccel_path_type_t type;
	if (path_from_uri(NULL, &type, path) != VACCEL_OK)
		return false;

	if (type == VACCEL_PATH_REMOTE_FILE)
		return net_path_is_url(path);

	return false;
}

int path_type(const char *path, vaccel_path_type_t *type)
{
	if (!path || !type)
		return VACCEL_EINVAL;

	if (path_is_url(path)) {
		*type = VACCEL_PATH_REMOTE_FILE;
		return VACCEL_OK;
	}

	if (fs_path_is_dir(path)) {
		*type = VACCEL_PATH_LOCAL_DIR;
		return VACCEL_OK;
	}

	if (fs_path_is_file(path)) {
		*type = VACCEL_PATH_LOCAL_FILE;
		return VACCEL_OK;
	}

	return VACCEL_ENOTSUP;
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

int path_file_name(const char *path, char *name, size_t size, char **alloc_name)
{
	if (!path || (!name && !alloc_name) || (!size && !alloc_name))
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
	if (alloc_name == NULL) {
		if (name_len >= size) {
			vaccel_error("Filename %s does not fit into buf",
				     name_pos);
			return VACCEL_ENAMETOOLONG;
		}
		strncpy(name, name_pos, size);
	} else {
		*alloc_name = strndup(name_pos, name_len);
		if (!*alloc_name)
			return VACCEL_ENOMEM;
	}

	return VACCEL_OK;
}

int path_file_name_add_random_suffix(char *path, size_t size, size_t *ext_len,
				     const char *base_path, const char *suffix)
{
	if (!path || !ext_len || !base_path || !suffix || !size)
		return VACCEL_EINVAL;

	/* Extract filename */
	char *last_slash = strrchr(base_path, '/');
	const char *name = last_slash ? (last_slash + 1) : base_path;

	/* Extract extension */
	const char *ext = strrchr(name, '.');

	size_t base_len = ext ? (size_t)(ext - base_path) : strlen(base_path);
	*ext_len = ext ? strlen(ext) : 0;
	size_t path_len = base_len + strlen(suffix) + *ext_len + 1;
	if (path_len > size) {
		vaccel_error("Suffixed path for %s does not fit into buf",
			     base_path);
		return VACCEL_ENAMETOOLONG;
	}

	/* Add path suffix before the extension (if any) */
	int ret = snprintf(path, size, "%.*s%s%s", (int)base_len, base_path,
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
