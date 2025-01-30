// SPDX-License-Identifier: Apache-2.0

#define _POSIX_C_SOURCE 200809L

#include "vaccel.h"
#include <assert.h>
#include <dlfcn.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void *vacceldl = NULL;

__attribute__((constructor)) void load_vaccel(void)
{
	/* Skip bootstrap of libvaccel so we can load libvaccel itself before
	 * loading any plugins. This is necessary to avoid unresolved symbols.
	 * See:
	 * https://mail.python.org/pipermail/python-dev/2002-May/023923.html */
	if (setenv("VACCEL_BOOTSTRAP_ENABLED", "0", 1)) {
		fprintf(stderr, "Error: Could not set env\n");
		exit(errno);
	}

	printf("Loading libvaccel\n");
	vacceldl = dlopen("libvaccel.so", RTLD_LAZY | RTLD_GLOBAL);
	if (!vacceldl) {
		fprintf(stderr, "Error: Could not open libvaccel\n");
		exit(VACCEL_ENOENT);
	}

	int ret = vaccel_bootstrap();
	if (ret)
		exit(ret);
}

__attribute__((destructor)) static void unload_vaccel(void)
{
	dlclose(vacceldl);
}
