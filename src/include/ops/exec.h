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

#ifndef __VACCEL_EXEC_OPS_H__
#define __VACCEL_EXEC_OPS_H__

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

struct vaccel_session;
struct vaccel_arg;
struct vaccel_shared_object;

int vaccel_exec(struct vaccel_session *sess, const char *library,
                const char *fn_symbol, struct vaccel_arg *read,
                size_t nr_read, struct vaccel_arg *write, size_t nr_write);

int vaccel_exec_with_resource(struct vaccel_session *sess, struct vaccel_shared_object *object,
                const char *fn_symbol, struct vaccel_arg *read,
                size_t nr_read, struct vaccel_arg *write, size_t nr_write);

#ifdef __cplusplus
}
#endif

#endif /* __VACCEL_EXEC_OPS_H__ */
