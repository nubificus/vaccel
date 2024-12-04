// SPDX-License-Identifier: Apache-2.0

#include <cstring>
#define _POSIX_C_SOURCE 200809L

#include "utils/fs.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

extern "C" {

auto abs_path(const char *root, const char *file) -> char *
{
	std::string const abs_path =
		std::string(root) + '/' + std::string(file);
	return strdup(abs_path.c_str());
}

auto read_file_from_dir(const char *dir, const char *path,
			size_t *len) -> unsigned char *
{
	char fpath[1024];

	snprintf(fpath, 1024, "%s/%s", dir, path);
	unsigned char *ptr;
	int const ret = fs_file_read_mmap(fpath, (void **)&ptr, len);
	if (ret != 0)
		fprintf(stderr, "Could not mmap %s", fpath);

	return ptr;
}
}
