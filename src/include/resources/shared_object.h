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

#include <stddef.h>
#include <stdbool.h>

#include <vaccel_id.h>
#include <vaccel_file.h>

#ifdef __cplusplus
extern "C" {
#endif

struct vaccel_resource;

struct vaccel_shared_object {
	/* Underlying resource object */
	struct vaccel_resource *resource;

	/* The protobuf file of the shared object */
	struct vaccel_file file;

	/* Plugin specific data */
	void *plugin_data;
};

int vaccel_shared_object_new(
	struct vaccel_shared_object *object,
	const char *path
);

int vaccel_shared_object_new_from_buffer(
	struct vaccel_shared_object *object,
	const uint8_t *buff,
	size_t size
);

int vaccel_shared_object_destroy(struct vaccel_shared_object *object);

vaccel_id_t vaccel_shared_object_get_id(
	const struct vaccel_shared_object *object
);

const uint8_t *vaccel_shared_object_get(
	struct vaccel_shared_object *object, size_t *len
);

#ifdef __cplusplus
}
#endif
