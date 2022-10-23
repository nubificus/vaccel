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

#include <vaccel_id.h>
#include <vaccel_file.h>

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

struct vaccel_resource;

/* A struct representing a PyTorch saved model
 *
 * A PyTorch saved model consists of:
 * 1) A protobuf file containing the model
 * 2) A set of checkpoints containing values for the variables of the
 *    model.
 */
struct vaccel_torch_saved_model {
	/* Underlying resource object */
	struct vaccel_resource *resource;

	/* The path of the saved model in disk */
	const char *path;

	/* Model file */
	struct vaccel_file model;

};

struct vaccel_torch_saved_model *vaccel_torch_saved_model_new(void);

int vaccel_torch_saved_model_set_path(
	struct vaccel_torch_saved_model *model,
	const char *path
);

const char *vaccel_torch_saved_model_get_path(struct vaccel_torch_saved_model *model);

int vaccel_torch_saved_model_set_model(
	struct vaccel_torch_saved_model *model,
	const uint8_t *ptr, size_t len
);

const uint8_t *vaccel_torch_saved_model_get_model(
	struct vaccel_torch_saved_model *model,
	size_t *len
);

int vaccel_torch_saved_model_set_checkpoint(
	struct vaccel_torch_saved_model *model,
	const uint8_t *ptr, size_t len
);

const uint8_t *vaccel_torch_saved_model_get_checkpoint(
	struct vaccel_torch_saved_model *model,
	size_t *len
);

int vaccel_torch_saved_model_set_var_index(
	struct vaccel_torch_saved_model *model,
	const uint8_t *ptr, size_t len
);

const uint8_t *vaccel_torch_saved_model_get_var_index(
	struct vaccel_torch_saved_model *model,
	size_t *len
);

int vaccel_torch_saved_model_register(struct vaccel_torch_saved_model *model);

int vaccel_torch_saved_model_destroy(struct vaccel_torch_saved_model *model);

vaccel_id_t vaccel_torch_saved_model_id(const struct vaccel_torch_saved_model *model);

#ifdef __cplusplus
}
#endif
