// SPDX-License-Identifier: Apache-2.0

#define _POSIX_C_SOURCE 200809L

#include "vaccel.h"
#include <errno.h>
#include <limits.h>
#include <linux/limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* Directory for holding resources related with the runtime */
static char rundir[PATH_MAX];
static struct vaccel_config config;

static int create_rundir(void)
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

static int destroy_rundir(void)
{
	/* Try to cleanup the rundir. At the moment, we do not fail
	 * if this fails, we just warn the user */
	if (fs_dir_remove(rundir))
		vaccel_warn("Could not remove rundir %s: %s", rundir,
			    strerror(errno));

	return VACCEL_OK;
}

const char *vaccel_rundir(void)
{
	return rundir;
}

const struct vaccel_config *vaccel_config(void)
{
	return &config;
}

__attribute__((constructor)) static void vaccel_init(void)
{
	int ret = VACCEL_EINVAL;

	ret = vaccel_config_init_from_env(&config);
	if (ret)
		exit(ret);

	vaccel_log_init();

	vaccel_debug("Initializing vAccel");
	vaccel_info("vAccel %s", VACCEL_VERSION);

	vaccel_config_print_debug(&config);

	ret = create_rundir();
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

	ret = plugins_bootstrap();
	if (ret) {
		vaccel_error("Could not bootstrap plugins system");
		exit(ret);
	}

	if (!config.plugins)
		return;

	ret = vaccel_plugin_parse_and_load(config.plugins);
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
	plugins_cleanup();
	destroy_rundir();
	vaccel_config_release(&config);
}
