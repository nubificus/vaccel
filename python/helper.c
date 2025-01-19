// SPDX-License-Identifier: Apache-2.0

#define _POSIX_C_SOURCE 200809L

#include "vaccel.h"
#include <assert.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void *vacceldl = NULL;
char *plugins = NULL;

__attribute__((constructor)) void load_vaccel(void)
{
	/* Get a copy of the plugin env variable */
	const char *plugins_env = getenv("VACCEL_PLUGINS");
	if (plugins_env)
		plugins = strdup(plugins_env);

	/* Do not allow libvaccel.so to load the plugins
	   as we get unresolved symbols. See:
	   https://mail.python.org/pipermail/python-dev/2002-May/023923.html */
	unsetenv("VACCEL_PLUGINS");

	printf("Loading libvaccel\n");
	vacceldl = dlopen("libvaccel.so", RTLD_LAZY | RTLD_GLOBAL);
	if (!vacceldl) {
		fprintf(stderr, "Could not open libvaccel\n");
		exit(1);
	}

	/* Instead, load the plugins now */
	printf("Loading plugins\n");
	vaccel_plugin_parse_and_load(plugins);
}

__attribute__((destructor)) static void unload_vaccel(void)
{
	printf("Shutting down vAccel\n");
	/* This will unregister and free resources of the plugins */
	dlclose(vacceldl);
	if (plugins)
		free(plugins);
}
