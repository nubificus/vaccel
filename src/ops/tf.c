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

#include "error.h"
#include "plugin.h"
#include "log.h"
#include "vaccel_ops.h"
#include "genop.h"
#include "tf.h"
#include "session.h"
#include "vaccel_prof.h"

#include <stdlib.h>
#include <string.h>

/* Create a new TensorFlow buffer
 *
 * This will take ownership of the data and it will destroy them,
 * if set, during destruction calling `free`
 */
struct vaccel_tf_buffer *vaccel_tf_buffer_new(void *data, size_t size)
{
	struct vaccel_tf_buffer *ret = calloc(1, sizeof(*ret));
	if (!ret)
		return NULL;

	ret->data = data;
	ret->size = size;

	return ret;
}

/* Destroy a TensorFlow buffer
 *
 * If the buffer's data is set, this will deallocate it
 */
void vaccel_tf_buffer_destroy(struct vaccel_tf_buffer *buffer)
{
	if (buffer->data)
		free(buffer->data);

	free(buffer);
}

/* Take ownership of buffer's data
 *
 * Useful if we want to destroy the buffer but not the
 * encapsulated data
 */
void *vaccel_tf_buffer_take_data(struct vaccel_tf_buffer *buffer, size_t *size)
{
	void *ptr;
	if (!buffer)
		return NULL;

	*size = buffer->size;
	ptr = buffer->data;
	buffer->data = NULL;
	buffer->size = 0;

	return ptr;
}

/* Get a read-only pointer to the buffer's data */
void *vaccel_tf_buffer_get_data(
	const struct vaccel_tf_buffer *buffer,
	size_t *size
) {
	if (!buffer)
		return NULL;

	*size = buffer->size;
	return buffer->data;
}

/* Creates a new TensorFlow node
 *
 * The name will be copied inside the node
 */
struct vaccel_tf_node *vaccel_tf_node_new(const char *name, long long int id) {
	struct vaccel_tf_node *ret = malloc(sizeof(*ret));
	if (!ret)
		return NULL;

	ret->name = strdup(name);
	if (!ret->name)
		goto free_node;

	ret->id = id;
	return ret;

free_node:
	free(ret);
	return NULL;
}

/* Destroy a TensorFlow node */
void vaccel_tf_node_destroy(struct vaccel_tf_node *node)
{
	if (!node)
		return;

	if (node->name)
		free(node->name);

	free(node);
}

/* Get the name of a node */
const char *vaccel_tf_node_get_name(struct vaccel_tf_node *node)
{
	if (!node)
		return NULL;

	return node->name;
}

/* Get the id of a node
 *
 * This will return the id of the node if the node is non-NULL
 * or a negative value otherwise
 */
long long int vaccel_tf_node_get_id(struct vaccel_tf_node *node)
{
	if (!node)
		return -VACCEL_EINVAL;

	return node->id;
}

struct vaccel_tf_tensor *
vaccel_tf_tensor_new(int nr_dims, uint32_t *dims, enum vaccel_tf_data_type type)
{
	struct vaccel_tf_tensor *ret;

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

	return ret;
}

struct vaccel_tf_tensor *
vaccel_tf_tensor_allocate(
	int nr_dims, uint32_t *dims,
	enum vaccel_tf_data_type type,
	size_t total_size
) {
	struct vaccel_tf_tensor *ret =
		vaccel_tf_tensor_new(nr_dims, dims, type);
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

int vaccel_tf_tensor_destroy(struct vaccel_tf_tensor *tensor)
{
	if (!tensor)
		return VACCEL_EINVAL;

	free(tensor->dims);

	if (tensor->data && tensor->owned)
		free(tensor->data);

	free(tensor);
	return VACCEL_OK;
}

int vaccel_tf_tensor_set_data(
	struct vaccel_tf_tensor *tensor,
	void *data,
	size_t size
) {
	if (!tensor)
		return VACCEL_EINVAL;

	if (tensor->data && tensor->owned)
		free(tensor->data);

	tensor->data = data;
	tensor->size = size;

	return VACCEL_OK;
}

void *vaccel_tf_tensor_get_data(struct vaccel_tf_tensor *tensor)
{
	if (!tensor)
		return NULL;

	return tensor->data;
}

struct vaccel_prof_region tf_load_stats =
	VACCEL_PROF_REGION_INIT("vaccel_tf_session_load");

int vaccel_tf_session_load(
	struct vaccel_session *session,
	struct vaccel_tf_saved_model *model,
	struct vaccel_tf_status *status)
{
	int ret;
	vaccel_debug("TensorFlow: load graph");

	if (!session) {
		vaccel_debug("Invalid session");
		return VACCEL_EINVAL;
	}

	vaccel_prof_region_start(&tf_load_stats);

	// Get implementation
	int (*plugin_op)(
		struct vaccel_session *,
		struct vaccel_tf_saved_model *,
		struct vaccel_tf_status *
	) = get_plugin_op(VACCEL_TF_SESSION_LOAD, session->hint);
	if (!plugin_op) {
		ret = VACCEL_ENOTSUP;
		goto out;
	}

	ret = plugin_op(session, model, status);

out:
	vaccel_prof_region_stop(&tf_load_stats);
	return ret;
}

struct vaccel_prof_region tf_session_run_stats =
	VACCEL_PROF_REGION_INIT("vaccel_tf_session_run");

int vaccel_tf_session_run(
	struct vaccel_session *session,
        const struct vaccel_tf_saved_model *model, const struct vaccel_tf_buffer *run_options,
        const struct vaccel_tf_node *in_nodes, struct vaccel_tf_tensor *const *in, int nr_inputs,
        const struct vaccel_tf_node *out_nodes, struct vaccel_tf_tensor **out, int nr_outputs,
        struct vaccel_tf_status *status)
{
	int ret;
	vaccel_debug("TensorFlow: run graph");

	if (!session) {
		vaccel_debug("Invalid session");
		return VACCEL_EINVAL;
	}

	vaccel_prof_region_start(&tf_session_run_stats);

	// Get implementation
	int (*plugin_op)(
		struct vaccel_session *,
		const struct vaccel_tf_saved_model *, const struct vaccel_tf_buffer *,
		const struct vaccel_tf_node *, struct vaccel_tf_tensor *const *, int,
		const struct vaccel_tf_node *, struct vaccel_tf_tensor **, int,
		struct vaccel_tf_status *
	) = get_plugin_op(VACCEL_TF_SESSION_RUN, session->hint);
	if (!plugin_op) {
		ret = VACCEL_ENOTSUP;
		goto out;
	}

	ret = plugin_op(session, model, run_options, in_nodes, in, nr_inputs,
			out_nodes, out, nr_outputs, status);

out:
	vaccel_prof_region_stop(&tf_session_run_stats);
	return ret;
}

struct vaccel_prof_region tf_session_delete_stats =
	VACCEL_PROF_REGION_INIT("vaccel_tf_session_delete");

int vaccel_tf_session_delete(
	struct vaccel_session *session,
	struct vaccel_tf_saved_model *model,
	struct vaccel_tf_status *status
) {
	int ret;
	vaccel_debug("TensorFlow: delete session");

	if (!session) {
		vaccel_debug("Invalid vAccel session");
		return VACCEL_EINVAL;
	}

	int (*plugin_op)(
		struct vaccel_session *, struct vaccel_tf_saved_model *,
		struct vaccel_tf_status *
	) = get_plugin_op(VACCEL_TF_SESSION_DELETE, session->hint);
	if (!plugin_op) {
		ret = VACCEL_ENOTSUP;
		goto out;
	}

	ret = plugin_op(session, model, status);

out:
	vaccel_prof_region_stop(&tf_session_delete_stats);
	return ret;
}

__attribute__((constructor))
static void vaccel_tf_ops_init(void)
{
}

__attribute__((destructor))
static void vaccel_tf_ops_fini(void)
{
	vaccel_prof_region_print(&tf_load_stats);
	vaccel_prof_region_print(&tf_session_run_stats);
	vaccel_prof_region_print(&tf_session_delete_stats);
}
