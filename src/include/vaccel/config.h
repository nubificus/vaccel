// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "log.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

struct vaccel_config {
	/* plugins to load */
	char *plugins;

	/* log level of messages */
	vaccel_log_level_t log_level;

	/* log file to print output to */
	char *log_file;

	/* if true profiling is enabled */
	bool profiling_enabled;

	/* if true plugins' vaccel version check is skipped */
	bool version_ignore;
};

/* Initialize config */
int vaccel_config_init(struct vaccel_config *config, const char *plugins,
		       vaccel_log_level_t log_level, const char *log_file,
		       bool profiling_enabled, bool version_ignore);

/* Initialize config from environment variables */
int vaccel_config_init_from_env(struct vaccel_config *config);

/* Initialize config from existing config */
int vaccel_config_init_from(struct vaccel_config *config,
			    struct vaccel_config *config_src);

/* Release config data */
int vaccel_config_release(struct vaccel_config *config);

/* Allocate and initialize config */
int vaccel_config_new(struct vaccel_config **config, const char *plugins,
		      vaccel_log_level_t log_level, const char *log_file,
		      bool profiling_enabled, bool version_ignore);

/* Allocate and initialize config from environment variables */
int vaccel_config_from_env(struct vaccel_config **config);

/* Allocate and initialize config from existing config */
int vaccel_config_from(struct vaccel_config **config,
		       struct vaccel_config *config_src);

/* Release config data and free config created with
 * vaccel_config_new*() or vaccel_config_from*() */
int vaccel_config_delete(struct vaccel_config *config);

/* Print config data as debug output */
void vaccel_config_print_debug(struct vaccel_config *config);

#ifdef __cplusplus
}
#endif
