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

#include "error.h"
#include "vaccel_id.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	VACCEL_RES_TF_MODEL = 0,
	VACCEL_RES_TF_SAVED_MODEL,
	VACCEL_RES_CAFFE_MODEL,
	VACCEL_RES_SHARED_OBJ,
	VACCEL_RES_TORCH_MODEL,
	VACCEL_RES_TORCH_SAVED_MODEL,
	VACCEL_RES_MAX
} vaccel_resource_t;

struct vaccel_resource;

vaccel_id_t resource_get_id(struct vaccel_resource *resource);

#ifdef __cplusplus
}
#endif
