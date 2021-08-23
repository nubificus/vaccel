/*
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

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
