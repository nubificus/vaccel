#include "error.h"
#include "log.h"

#include <stdlib.h>
#include <string.h>
#include <slog.h>

#define VACCEL_DEBUG_LVL_ERROR 1
#define VACCEL_DEBUG_LVL_WARN  2
#define VACCEL_DEBUG_LVL_INFO  3
#define VACCEL_DEBUG_LVL_DEBUG 4

static void set_debug_level(void)
{
	char *env = getenv("VACCEL_DEBUG_LEVEL");
	if (!env)
		return;

	int level = atoi(env);
	int nEnabledLevels = 0;
	switch (level) {
		/* FALLTHRU */
		case VACCEL_DEBUG_LVL_DEBUG:
			nEnabledLevels |= SLOG_DEBUG;
		/* FALLTHRU */
		case VACCEL_DEBUG_LVL_INFO:
			nEnabledLevels |= SLOG_INFO;
		/* FALLTHRU */
		case VACCEL_DEBUG_LVL_WARN:
			nEnabledLevels |= SLOG_WARN;
		/* FALLTHRU */
		case VACCEL_DEBUG_LVL_ERROR:
			nEnabledLevels |= SLOG_ERROR;
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

	char *env = getenv("VACCEL_LOG_FILE");
	if (!env)
		return;

	slog_config_get(&cfg);
	strncpy(cfg.sFileName, env, SLOG_NAME_MAX-1);

	if (strncpy(cfg.sFileName, "/dev/stdout", SLOG_NAME_MAX-1)
			&& strncpy(cfg.sFileName, "/dev/stderr", SLOG_NAME_MAX-1)) {
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
