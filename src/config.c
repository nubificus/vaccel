// SPDX-License-Identifier: Apache-2.0

#define _POSIX_C_SOURCE 200809L

#include "config.h"
#include "error.h"
#include "log.h"
#include "utils/str.h"
#include <errno.h>
#include <limits.h>
#include <linux/limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { DEC_BASE = 10 };

static int config_ulong_from_env(unsigned long *config_ulong,
				 const char *ulong_env,
				 unsigned long ulong_default)
{
	char *env = getenv(ulong_env);
	if (!env) {
		*config_ulong = ulong_default;
		return VACCEL_OK;
	}

	errno = 0;
	unsigned long int ulong = strtoul(env, NULL, DEC_BASE);
	if (errno)
		return VACCEL_EINVAL;

	*config_ulong = ulong;

	return VACCEL_OK;
}

static int config_bool_from_env(bool *config_bool, const char *bool_env,
				bool bool_default)
{
	unsigned long bool_ulong;
	int ret = config_ulong_from_env(&bool_ulong, bool_env,
					(unsigned long)bool_default);
	if (ret)
		return ret;

	*config_bool = (bool_ulong != 0);

	return VACCEL_OK;
}

static int config_str_from_env(char **config_str, const char *str_env,
			       const char *str_default)
{
	const char *env = getenv(str_env);
	if (!env) {
		if (!str_default) {
			*config_str = NULL;
			return VACCEL_OK;
		}
		*config_str = strdup(str_default);
	} else {
		*config_str = strdup(env);
	}

	if (!*config_str)
		return VACCEL_ENOMEM;

	return VACCEL_OK;
}

static inline const char *config_deprecate_env(const char *old, const char *new)
{
	if (getenv(old) && !getenv(new)) {
		fprintf(stderr, "Warning: %s is deprecated. Use %s instead.\n",
			old, new);
		return old;
	}

	return new;
}

int vaccel_config_init(struct vaccel_config *config, const char *plugins,
		       vaccel_log_level_t log_level, const char *log_file,
		       bool profiling_enabled, bool version_ignore)
{
	if (!config)
		return VACCEL_EINVAL;

	config->plugins = plugins ? strdup(plugins) : NULL;
	if (plugins && !config->plugins)
		return VACCEL_ENOMEM;
	config->log_level = log_level;
	config->log_file = log_file ? strdup(log_file) : NULL;
	if (log_file && !config->log_file)
		return VACCEL_ENOMEM;
	config->profiling_enabled = profiling_enabled;
	config->version_ignore = version_ignore;

	return VACCEL_OK;
}

int vaccel_config_init_from_env(struct vaccel_config *config)
{
	if (!config)
		return VACCEL_EINVAL;

	unsigned long log_level_ul;
	const char *log_level_env = config_deprecate_env(
		CONFIG_LOG_LEVEL_OLD_ENV, CONFIG_LOG_LEVEL_ENV);
	int ret = config_ulong_from_env(&log_level_ul, log_level_env,
					(unsigned int)CONFIG_LOG_LEVEL_DEFAULT);
	if (ret)
		return ret;
	config->log_level = (vaccel_log_level_t)log_level_ul;

	ret = config_str_from_env(&config->log_file, CONFIG_LOG_FILE_ENV,
				  CONFIG_LOG_FILE_DEFAULT);
	if (ret)
		return ret;

	const char *plugins_env = config_deprecate_env(CONFIG_PLUGINS_OLD_ENV,
						       CONFIG_PLUGINS_ENV);
	ret = config_str_from_env(&config->plugins, plugins_env,
				  CONFIG_PLUGINS_DEFAULT);
	if (ret)
		return ret;

	ret = config_bool_from_env(&config->profiling_enabled,
				   CONFIG_PROFILING_ENABLED_ENV,
				   CONFIG_PROFILING_ENABLED_DEFAULT);
	if (ret)
		return ret;

	const char *version_ignore_env = config_deprecate_env(
		CONFIG_VERSION_IGNORE_OLD_ENV, CONFIG_VERSION_IGNORE_ENV);
	ret = config_bool_from_env(&config->version_ignore, version_ignore_env,
				   CONFIG_VERSION_IGNORE_DEFAULT);
	if (ret)
		return ret;

	return VACCEL_OK;
}

int vaccel_config_init_from(struct vaccel_config *config,
			    struct vaccel_config *config_src)
{
	if (!config || !config_src)
		return VACCEL_EINVAL;

	config->plugins = config_src->plugins ? strdup(config_src->plugins) :
						NULL;
	if (config_src->plugins && !config->plugins)
		return VACCEL_ENOMEM;
	config->log_level = config_src->log_level;
	config->log_file = config_src->log_file ? strdup(config_src->log_file) :
						  NULL;
	if (config_src->log_file && !config->log_file)
		return VACCEL_ENOMEM;
	config->profiling_enabled = config_src->profiling_enabled;
	config->version_ignore = config_src->version_ignore;

	return VACCEL_OK;
}

int vaccel_config_release(struct vaccel_config *config)
{
	if (!config)
		return VACCEL_EINVAL;

	if (config->plugins)
		free(config->plugins);
	if (config->log_file)
		free(config->log_file);

	config->plugins = CONFIG_PLUGINS_DEFAULT;
	config->log_level = CONFIG_LOG_LEVEL_DEFAULT;
	config->log_file = CONFIG_LOG_FILE_DEFAULT;
	config->profiling_enabled = CONFIG_PROFILING_ENABLED_DEFAULT;
	config->version_ignore = CONFIG_VERSION_IGNORE_DEFAULT;

	return VACCEL_OK;
}

int vaccel_config_new(struct vaccel_config **config, const char *plugins,
		      vaccel_log_level_t log_level, const char *log_file,
		      bool profiling_enabled, bool version_ignore)
{
	if (!config)
		return VACCEL_EINVAL;

	struct vaccel_config *c =
		(struct vaccel_config *)malloc(sizeof(struct vaccel_config));
	if (!c)
		return VACCEL_ENOMEM;

	int ret = vaccel_config_init(c, plugins, log_level, log_file,
				     profiling_enabled, version_ignore);
	if (ret) {
		free(c);
		return ret;
	}

	*config = c;

	return VACCEL_OK;
}

int vaccel_config_from_env(struct vaccel_config **config)
{
	if (!config)
		return VACCEL_EINVAL;

	struct vaccel_config *c =
		(struct vaccel_config *)malloc(sizeof(struct vaccel_config));
	if (!c)
		return VACCEL_ENOMEM;

	int ret = vaccel_config_init_from_env(c);
	if (ret) {
		free(c);
		return ret;
	}

	*config = c;

	return VACCEL_OK;
}

int vaccel_config_from(struct vaccel_config **config,
		       struct vaccel_config *config_src)
{
	if (!config || !config_src)
		return VACCEL_EINVAL;

	struct vaccel_config *c =
		(struct vaccel_config *)malloc(sizeof(struct vaccel_config));
	if (!c)
		return VACCEL_ENOMEM;

	int ret = vaccel_config_init_from(c, config_src);
	if (ret) {
		free(c);
		return ret;
	}

	*config = c;

	return VACCEL_OK;
}

int vaccel_config_delete(struct vaccel_config *config)
{
	int ret = vaccel_config_release(config);
	if (ret)
		return ret;

	free(config);

	return VACCEL_OK;
}

void vaccel_config_print_debug(struct vaccel_config *config)
{
	if (!config)
		return;

	char log_level_str[NAME_MAX];
	strncpy(log_level_str, vaccel_log_level_to_base_str(config->log_level),
		NAME_MAX);
	vaccel_str_to_lower(log_level_str, NAME_MAX, NULL);

	vaccel_debug("Config:");
	vaccel_debug("  plugins = %s", config->plugins);
	vaccel_debug("  log_level = %s", log_level_str);
	vaccel_debug("  log_file = %s", config->log_file);
	vaccel_debug("  profiling_enabled = %s",
		     config->profiling_enabled ? "true" : "false");
	vaccel_debug("  version_ignore = %s",
		     config->version_ignore ? "true" : "false");
}
