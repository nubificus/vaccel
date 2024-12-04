// SPDX-License-Identifier: Apache-2.0

#define _POSIX_C_SOURCE 200809L

#include "vaccel.h"
#include <dlfcn.h>
#include <errno.h>
#include <limits.h>
#include <linux/limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* Runtime directory for holding resources related with the
 * runtime */
static char rundir[PATH_MAX];

static int load_backend_plugin(const char *path)
{
	int ret;
	struct vaccel_plugin *plugin;
	struct vaccel_plugin **plugin_p;

	void *dl = dlopen(path, RTLD_LAZY);
	if (!dl) {
		vaccel_error("Could not dlopen plugin %s: %s", path, dlerror());
		return VACCEL_ENOENT;
	}

	plugin_p = dlsym(dl, "vaccel_plugin");
	if (!plugin_p) {
		vaccel_error("Not a vaccel plugin: %s", path);
		ret = VACCEL_ELIBBAD;
		goto close_dl;
	}

	plugin = *plugin_p;
	ret = register_plugin(plugin);
	if (ret != VACCEL_OK) {
		vaccel_error("Could not register plugin %s", path);
		goto close_dl;
	}

	/* Initialize the plugin */
	ret = plugin->info->init();
	if (ret != VACCEL_OK) {
		vaccel_error("Could not initialize plugin %s",
			     plugin->info->name);
		goto close_dl;
	}

	/* Keep dl handle so we can later close the library */
	plugin->dl_handle = dl;

	vaccel_debug("Loaded plugin %s from %s", plugin->info->name, path);

	return VACCEL_OK;

close_dl:
	dlclose(dl);
	return ret;
}

int vaccel_plugin_load(const char *path)
{
	if (path == NULL)
		return VACCEL_EINVAL;
	if (!fs_path_is_file(path))
		return VACCEL_ENOENT;
	return load_backend_plugin(path);
}

static int load_backend_plugins(char *plugins)
{
	char *plugin;
	int ret = VACCEL_OK;

	char *plugins_tmp = strdup(plugins);
	if (!plugins_tmp)
		return VACCEL_ENOMEM;

	char *p = plugins_tmp;
	plugin = strtok(p, ":");
	while (plugin) {
		ret = load_backend_plugin(plugin);
		if (ret != VACCEL_OK)
			break;

		plugin = strtok(NULL, ":");
	}

	free(plugins_tmp);
	return ret;
}

static int create_vaccel_rundir(void)
{
	/* Generate uid_t string */
	char uid_str[NAME_MAX];
	int ret = snprintf(uid_str, NAME_MAX, "%ju", (uintmax_t)getuid());
	if (ret < 0) {
		vaccel_error("Could not generate uid string for %ju: %s",
			     (uintmax_t)getuid(), strerror(errno));
		return ret;
	}
	if (ret == NAME_MAX) {
		vaccel_error("Top-level rundir path too long", uid_str);
		return VACCEL_ENAMETOOLONG;
	}

	/* Generate parent vaccel dir string */
	ret = path_init_from_parts(rundir, PATH_MAX, "/run/user", uid_str,
				   "vaccel/", NULL);
	if (ret) {
		vaccel_error("Could not generate top-level rundir string");
		return ret;
	}

	/* Create unique (random) rundir under /run/user/<uid>/vaccel */
	ret = fs_dir_create_unique(rundir, PATH_MAX, NULL);
	if (ret) {
		vaccel_error("Could not create unique top-level rundir");
		return ret;
	}

	vaccel_debug("Created top-level rundir: %s", rundir);

	return VACCEL_OK;
}

static int cleanup_vaccel_rundir(void)
{
	/* Try to cleanup the rundir. At the moment, we do not fail
	 * if this fails, we just warn the user */
	if (fs_dir_remove(rundir))
		vaccel_warn("Could not remove rundir %s", rundir);

	return VACCEL_OK;
}

const char *vaccel_rundir(void)
{
	return rundir;
}

__attribute__((constructor)) static void vaccel_init(void)
{
	int ret = VACCEL_EINVAL;

	/* Initialize logger */
	vaccel_log_init();

	vaccel_debug("Initializing vAccel");
	vaccel_info("vAccel %s", VACCEL_VERSION);

	ret = create_vaccel_rundir();
	if (ret) {
		vaccel_error("Could not create rundir for vAccel");
		exit(ret);
	}

	ret = sessions_bootstrap();
	if (ret) {
		vaccel_error("Could not bootstrap sessions system");
		exit(ret);
	}

	ret = resources_bootstrap();
	if (ret) {
		vaccel_error("Could not bootstrap resources system");
		exit(ret);
	}

	/* initialize the backends system */
	plugins_bootstrap();

	/* find backend implementations and set them up */
	char *plugins = getenv("VACCEL_BACKENDS");
	if (!plugins)
		return;

	ret = load_backend_plugins(plugins);
	if (ret) {
		vaccel_error("Could not load backend plugins");
		exit(ret);
	}
}

__attribute__((destructor)) static void vaccel_fini(void)
{
	vaccel_debug("Shutting down vAccel");
	sessions_cleanup();
	resources_cleanup();
	plugins_shutdown();
	cleanup_vaccel_rundir();
}
