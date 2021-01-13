#include "common.h"
#include "log.h"

#include <slog.h>

static int nEnabledLevels = SLOG_FLAGS_ALL;
static const char *logfile = "/dev/stdout";

int vaccel_log_init(void)
{
	slog_init(logfile, nEnabledLevels, 0);
	return VACCEL_OK;
}

int vaccel_log_shutdown(void)
{
	slog_destroy();
	return VACCEL_OK;
}
