// SPDX-License-Identifier: Apache-2.0

#include "log.h"
#include "config.h"
#include "core.h"
#include "error.h"
#include <slog.h>
#include <string.h>

static void set_debug_level(void)
{
	const struct vaccel_config *config = vaccel_config();
	int nEnabledLevels = 0;

	switch (config->log_level) {
	case VACCEL_LOG_DEBUG:
		nEnabledLevels |= SLOG_DEBUG;
		/* fallthrough */
	case VACCEL_LOG_INFO:
		nEnabledLevels |= SLOG_INFO;
		/* fallthrough */
	case VACCEL_LOG_WARN:
		nEnabledLevels |= SLOG_WARN;
		/* fallthrough */
	case VACCEL_LOG_ERROR:
		nEnabledLevels |= SLOG_ERROR;
		/* fallthrough */
	default:
		break;
	}

	SLogConfig cfg;
	slog_config_get(&cfg);
	cfg.nFlags = nEnabledLevels;
	slog_config_set(&cfg);
}

static void set_log_file(void)
{
	SLogConfig cfg;
	const struct vaccel_config *config = vaccel_config();
	if (!config->log_file)
		return;

	slog_config_get(&cfg);
	strncpy(cfg.sFileName, config->log_file, SLOG_NAME_MAX - 1);

	if (strncmp(cfg.sFileName, "/dev/stdout", SLOG_NAME_MAX - 1) != 0 &&
	    strncmp(cfg.sFileName, "/dev/stderr", SLOG_NAME_MAX - 1) != 0) {
		cfg.nToFile = 1;
		cfg.nToScreen = 0;
	}

	slog_config_set(&cfg);
}

int vaccel_log_init(void)
{
	slog_init("/dev/stdout", 0, 0);

	set_debug_level();
	set_log_file();

	return VACCEL_OK;
}

int vaccel_log_shutdown(void)
{
	slog_destroy();
	return VACCEL_OK;
}
