// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <stdbool.h>
#include <stddef.h>

#include <vaccel_file.h>
#include <vaccel_id.h>

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

int vaccel_shared_object_new(struct vaccel_shared_object *object,
			     const char *path);

int vaccel_shared_object_new_from_buffer(struct vaccel_shared_object *object,
					 const uint8_t *buff, size_t size);

int vaccel_shared_object_destroy(struct vaccel_shared_object *object);

vaccel_id_t
vaccel_shared_object_get_id(const struct vaccel_shared_object *object);

const uint8_t *vaccel_shared_object_get(struct vaccel_shared_object *object,
					size_t *len);

#ifdef __cplusplus
}
#endif
