#pragma once

#include <stddef.h>
#include <stdbool.h>

#include <vaccel_id.h>
#include <vaccel_file.h>

#ifdef __cplusplus
extern "C" {
#endif

struct vaccel_resource;

struct vaccel_tf_model {
	/* Underlying resource object */
	struct vaccel_resource *resource;

	/* The protobuf file of the model */
	struct vaccel_file file;

	/* Plugin specific data */
	void *plugin_data;
};

int vaccel_tf_model_new(
	struct vaccel_tf_model *model,
	const char *path
);

int vaccel_tf_model_new_from_buffer(
	struct vaccel_tf_model *model,
	const uint8_t *buff,
	size_t size
);

int vaccel_tf_model_destroy(struct vaccel_tf_model *model);

vaccel_id_t vaccel_tf_model_get_id(
	const struct vaccel_tf_model *model
);

#ifdef __cplusplus
}
#endif
