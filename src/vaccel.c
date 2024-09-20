// SPDX-License-Identifier: Apache-2.0

#define _POSIX_C_SOURCE 200809L

#include "plugin.h"
#include "log.h"
#include "resources.h"
#include "session.h"
#include "utils.h"
#include "vaccel.h"

#include <dlfcn.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

enum { MAX_RUNDIR_PATH = 1024 };

/* Runtime directory for holding resources related with the
 * runtime */
static char rundir[MAX_RUNDIR_PATH];

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
	if (ret != VACCEL_OK)
		goto close_dl;

	/* Initialize the plugin */
	ret = plugin->info->init();
	if (ret != VACCEL_OK)
		goto close_dl;

	/* Keep dl handle so we can later close the library */
	plugin->dl_handle = dl;

	vaccel_debug("Loaded plugin %s from %s", plugin->info->name, path);

	return VACCEL_OK;

close_dl:
	dlclose(dl);
	return ret;
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
	int ret = snprintf(rundir, MAX_RUNDIR_PATH, "/run/user/%u", getuid());
	if (ret == MAX_RUNDIR_PATH) {
		vaccel_fatal("rundir path '%s' too long", rundir);
		return VACCEL_ENAMETOOLONG;
	}

	if (!dir_exists(rundir)) {
		vaccel_debug(
			"User rundir does not exist. Will try to create it");
		if (!dir_exists("/run/user")) {
			vaccel_debug(
				"/run/user dir does not exist. Will try to create it first");
			ret = mkdir("/run/user", 0700);
			if (ret) {
				vaccel_fatal("Could not create user rundir: %s",
					     strerror(errno));
				return VACCEL_ENOENT;
			}
		}
		ret = mkdir(rundir, 0700);
		if (ret) {
			vaccel_fatal("Could not create user rundir: %s",
				     strerror(errno));
			return VACCEL_ENOENT;
		}

		vaccel_debug("Created user rundir %s", rundir);
	}

	ret = snprintf(rundir, MAX_RUNDIR_PATH, "/run/user/%u/vaccel.XXXXXX",
		       getuid());
	if (ret == MAX_RUNDIR_PATH) {
		vaccel_error("rundir path '%s' too big", rundir);
		return VACCEL_ENAMETOOLONG;
	}

	char *dir = mkdtemp(rundir);
	if (!dir) {
		vaccel_error("Could not initialize top-level rundir: %s", dir);
		return errno;
	}

	vaccel_debug("Created top-level rundir: %s", rundir);

	return VACCEL_OK;
}

static int cleanup_vaccel_rundir(void)
{
	/* Try to cleanup the rundir. At the moment, we do not fail
	 * if this fails, we just warn the user */
	if (cleanup_rundir(rundir))
		vaccel_warn("Could not cleanup rundir '%s'", rundir);

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

	load_backend_plugins(plugins);
}

__attribute__((destructor)) static void vaccel_fini(void)
{
	vaccel_debug("Shutting down vAccel");
	plugins_shutdown();
	resources_cleanup();
	sessions_cleanup();
	cleanup_vaccel_rundir();
}
