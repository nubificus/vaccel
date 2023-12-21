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
#define _POSIX_C_SOURCE 200809L

#include "error.h"
#include "plugin.h"
#include "log.h"
#include "vaccel_ops.h"
#include "genop.h"
#include "tflite.h"
#include "session.h"
#include "vaccel_prof.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

struct vaccel_tflite_tensor *
vaccel_tflite_tensor_new(
	int nr_dims, int32_t *dims,
	enum vaccel_tflite_type type
) {
	struct vaccel_tflite_tensor *ret;

	ret = calloc(1, sizeof(*ret));
	if (!ret)
		return NULL;

	ret->data_type = type;
	ret->nr_dims = nr_dims;
	ret->dims = calloc(nr_dims, sizeof(*dims));
	if (!ret->dims) {
		free(ret);
		return NULL;
	}

	if (dims)
		memcpy(ret->dims, dims, nr_dims * sizeof(*dims));

	ret->data = NULL;
	ret->size = 0;
	ret->owned = false;

	return ret;
}

struct vaccel_tflite_tensor *
vaccel_tflite_tensor_allocate(
	int nr_dims, int32_t *dims,
	enum vaccel_tflite_type type,
	size_t total_size
) {
	struct vaccel_tflite_tensor *ret =
		vaccel_tflite_tensor_new(nr_dims, dims, type);
	if (!ret)
		return NULL;

	if (!total_size)
		return ret;

	ret->data = malloc(total_size);
	if (!ret)
		goto free_tensor;

	ret->size = total_size;
	ret->owned = true;

	return ret;

free_tensor:
	free(ret);
	return NULL;
}

int vaccel_tflite_tensor_destroy(struct vaccel_tflite_tensor *tensor)
{
	if (!tensor)
		return VACCEL_EINVAL;

	free(tensor->dims);

	if (tensor->data && tensor->owned)
		free(tensor->data);

	free(tensor);
	return VACCEL_OK;
}

int vaccel_tflite_tensor_set_data(
	struct vaccel_tflite_tensor *tensor,
	void *data,
	size_t size
) {
	if (!tensor)
		return VACCEL_EINVAL;

	if (tensor->data && tensor->owned)
		free(tensor->data);

	tensor->data = data;
	tensor->size = size;
	tensor->owned = false;

	return VACCEL_OK;
}

void *vaccel_tflite_tensor_get_data(struct vaccel_tflite_tensor *tensor)
{
	if (!tensor)
		return NULL;

	return tensor->data;
}

struct vaccel_prof_region tflite_load_stats =
	VACCEL_PROF_REGION_INIT("vaccel_tflite_session_load");

int vaccel_tflite_session_load(
	struct vaccel_session *sess,
	struct vaccel_single_model *model
) {
	int ret;

	if (!sess) {
		vaccel_debug("Invalid session");
		return VACCEL_EINVAL;
	}

	vaccel_debug("session:%u Looking for plugin implementing tflite_session_load operation",
				 sess->session_id);


	vaccel_prof_region_start(&tflite_load_stats);

	// Get implementation
	int (*plugin_op)(
		struct vaccel_session *,
		struct vaccel_single_model *
	) = get_plugin_op(VACCEL_TFLITE_SESSION_LOAD, sess->hint);
	if (!plugin_op) {
		ret = VACCEL_ENOTSUP;
		goto out;
	}

	ret = plugin_op(sess, model);

out:
	vaccel_prof_region_stop(&tflite_load_stats);
	return ret;
}

struct vaccel_prof_region tflite_session_run_stats =
	VACCEL_PROF_REGION_INIT("vaccel_tflite_session_run");

int vaccel_tflite_session_run(
	struct vaccel_session *sess,
        const struct vaccel_single_model *model,
        struct vaccel_tflite_tensor *const *in, int nr_inputs,
	struct vaccel_tflite_tensor **out, int nr_outputs,
        uint8_t *status
) {
	int ret;

	if (!sess) {
		vaccel_debug("Invalid session");
		return VACCEL_EINVAL;
	}

	vaccel_debug("session:%u Looking for plugin implementing tflite_session_run operation",
				 sess->session_id);

	vaccel_prof_region_start(&tflite_session_run_stats);

	// Get implementation
	int (*plugin_op)(
		struct vaccel_session *,
		const struct vaccel_single_model *,
		struct vaccel_tflite_tensor *const *, int,
		struct vaccel_tflite_tensor **, int,
		uint8_t *
	) = get_plugin_op(VACCEL_TFLITE_SESSION_RUN, sess->hint);
	if (!plugin_op) {
		ret = VACCEL_ENOTSUP;
		goto out;
	}

	ret = plugin_op(sess, model, in, nr_inputs, out, nr_outputs, status);

out:
	vaccel_prof_region_stop(&tflite_session_run_stats);
	return ret;
}

struct vaccel_prof_region tflite_session_delete_stats =
	VACCEL_PROF_REGION_INIT("vaccel_tflite_session_delete");

int vaccel_tflite_session_delete(
	struct vaccel_session *sess,
	struct vaccel_single_model *model
) {
	int ret;

	if (!sess) {
		vaccel_debug("Invalid vAccel session");
		return VACCEL_EINVAL;
	}

	vaccel_debug("session:%u Looking for plugin implementing tflite_session_delete operation",
				 sess->session_id);

	int (*plugin_op)(
		struct vaccel_session *,
		struct vaccel_single_model *
	) = get_plugin_op(VACCEL_TFLITE_SESSION_DELETE, sess->hint);
	if (!plugin_op) {
		ret = VACCEL_ENOTSUP;
		goto out;
	}

	ret = plugin_op(sess, model);

out:
	vaccel_prof_region_stop(&tflite_session_delete_stats);
	return ret;
}

__attribute__((constructor))
static void vaccel_tflite_ops_init(void)
{
}

__attribute__((destructor))
static void vaccel_tflite_ops_fini(void)
{
	vaccel_prof_region_print(&tflite_load_stats);
	vaccel_prof_region_print(&tflite_session_run_stats);
	vaccel_prof_region_print(&tflite_session_delete_stats);
}
