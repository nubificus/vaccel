#ifndef __VACCEL_LOG__
#define __VACCEL_LOG__

#ifdef __cplusplus
extern "C" {
#endif

#include <slog.h>

int vaccel_log_init(void);
int vaccel_log_shutdown(void);

#define vaccel_info slog_info
#define vaccel_warn slog_warn
#define vaccel_debug slog_debug
#define vaccel_error slog_error
#define vaccel_trace slog_trace
#define vaccel_fatal slog_fatal

#ifdef __cplusplus
}
#endif

#endif /* __VACCEL_LOG__ */
