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

#pragma once

#include "vaccel_args.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct vaccel_session;

/* Call one of the supported functions, given an op code and a set of arbitrary
 * arguments */
int vaccel_genop(struct vaccel_session *sess, struct vaccel_arg *read,
		 int nr_read, struct vaccel_arg *write, int nr_write);

#ifdef __cplusplus
}
#endif
