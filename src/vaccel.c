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

static struct vaccel {
	/* true if vAccel has been initialized */
	bool initialized;

	/* object with user-configurable options */
	struct vaccel_config config;

	/* directory for vAccel-related runtime files */
	char rundir[PATH_MAX];
} vaccel;

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
	ret = path_init_from_parts(vaccel.rundir, PATH_MAX, "/run/user",
				   uid_str, "vaccel/", NULL);
	if (ret) {
		vaccel_error("Could not generate top-level rundir string");
		return ret;
	}

	/* Create unique (random) rundir under /run/user/<uid>/vaccel */
	ret = fs_dir_create_unique(vaccel.rundir, PATH_MAX, NULL);
	if (ret) {
		vaccel_error("Could not create unique top-level rundir");
		return ret;
	}

	vaccel_debug("Created top-level rundir: %s", vaccel.rundir);

	return VACCEL_OK;
}

static int destroy_rundir(void)
{
	/* Try to cleanup the rundir. At the moment, we do not fail
	 * if this fails, we just warn the user */
	if (fs_dir_remove(vaccel.rundir))
		vaccel_warn("Could not remove rundir %s: %s", vaccel.rundir,
			    strerror(errno));

	return VACCEL_OK;
}

static int do_cleanup(void);

static int do_bootstrap(void)
{
	int ret;

	if (vaccel.initialized) {
		vaccel_debug("Reloading vAccel");
		ret = do_cleanup();
		if (ret) {
			vaccel_error("Could not cleanup vAccel");
			return ret;
		}
	} else {
		// Workaround: slog_destroy() in slog 1.8.22 does not properly
		// reset internal state so avoid re-initializing
		vaccel_log_init();
	}

	vaccel_debug("Initializing vAccel");
	vaccel_info("vAccel %s", VACCEL_VERSION);

	vaccel_config_print_debug(&vaccel.config);

	ret = create_rundir();
	if (ret) {
		vaccel_error("Could not create root rundir");
		return ret;
	}

	ret = sessions_bootstrap();
	if (ret) {
		vaccel_error("Could not bootstrap sessions");
		return ret;
	}

	ret = resources_bootstrap();
	if (ret) {
		vaccel_error("Could not bootstrap resources");
		return ret;
	}

	ret = plugins_bootstrap();
	if (ret) {
		vaccel_error("Could not bootstrap plugins");
		return ret;
	}

	if (vaccel.config.plugins) {
		ret = vaccel_plugin_parse_and_load(vaccel.config.plugins);
		if (ret) {
			vaccel_error("Could not load plugins");
			return ret;
		}
	}

	vaccel.initialized = true;

	return VACCEL_OK;
}

int vaccel_bootstrap(void)
{
	int ret;

	if (vaccel.initialized) {
		ret = vaccel_config_release(&vaccel.config);
		if (ret) {
			vaccel_error("Could not release main config");
			return ret;
		}
	}

	ret = vaccel_config_init_from_env(&vaccel.config);
	if (ret) {
		fprintf(stderr, "Error: Could not init main config from env\n");
		return ret;
	}

	return do_bootstrap();
}

int vaccel_bootstrap_with_config(struct vaccel_config *config)
{
	int ret;

	if (vaccel.initialized) {
		ret = vaccel_config_release(&vaccel.config);
		if (ret) {
			vaccel_error("Could not release main config");
			return ret;
		}
	}

	ret = vaccel_config_init_from(&vaccel.config, config);
	if (ret) {
		fprintf(stderr, "Error: Could not init main config\n");
		return ret;
	}

	return do_bootstrap();
}

static int do_cleanup(void)
{
	int ret;

	vaccel_debug("Cleaning up vAccel");

	ret = sessions_cleanup();
	if (ret) {
		vaccel_error("Could not cleanup sessions");
		return ret;
	}

	ret = resources_cleanup();
	if (ret) {
		vaccel_error("Could not cleanup resources");
		return ret;
	}

	ret = plugins_cleanup();
	if (ret) {
		vaccel_error("Could not cleanup plugins");
		return ret;
	}

	ret = destroy_rundir();
	if (ret) {
		vaccel_error("Could not destroy root rundir");
		return ret;
	}

	ret = vaccel_log_shutdown();
	if (ret) {
		vaccel_error("Could not shutdown logging");
		return ret;
	}

	vaccel.initialized = false;

	return VACCEL_OK;
}

int vaccel_cleanup(void)
{
	if (!vaccel.initialized)
		return VACCEL_OK;

	int ret = do_cleanup();
	if (ret)
		return ret;

	ret = vaccel_config_release(&vaccel.config);
	if (ret) {
		vaccel_error("Could not release main config");
		return ret;
	}

	return VACCEL_OK;
}

bool vaccel_is_initialized(void)
{
	return vaccel.initialized;
}
const char *vaccel_rundir(void)
{
	return vaccel.rundir;
}

const struct vaccel_config *vaccel_config(void)
{
	return &vaccel.config;
}

__attribute__((constructor)) static void vaccel_init(void)
{
	vaccel.initialized = false;

	const char *bootstrap_env = getenv(VACCEL_BOOTSTRAP_ENABLED_ENV);
	if (bootstrap_env && strcmp(bootstrap_env, "0") == 0)
		return;

	int ret = vaccel_bootstrap();
	if (ret)
		exit(ret);
}

__attribute__((destructor)) static void vaccel_fini(void)
{
	const char *cleanup_env = getenv(VACCEL_CLEANUP_ENABLED_ENV);
	if (cleanup_env && strcmp(cleanup_env, "0") == 0)
		return;

	vaccel_cleanup();
}
