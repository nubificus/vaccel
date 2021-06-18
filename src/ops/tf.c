#include "error.h"
#include "plugin.h"
#include "log.h"
#include "vaccel_ops.h"
#include "genop.h"
#include "tf.h"

#include "session.h"

int vaccel_tf_model_load_graph(
	struct vaccel_session *session,
	struct vaccel_tf_saved_model *model,
	struct vaccel_tf_status *status)
{
	vaccel_debug("TensorFlow: load graph");

	if (!session) {
		vaccel_debug("Invalid session");
		return VACCEL_EINVAL;
	}

	// Get implementation
	int (*plugin_op)() = get_plugin_op(VACCEL_TF_MODEL_LOAD_GRAPH);
	if (!plugin_op)
		return VACCEL_ENOTSUP;

	return plugin_op(session, model, status);
}

int vaccel_tf_model_run(
	struct vaccel_session *session,
        const struct vaccel_tf_saved_model *model, const struct vaccel_tf_buffer *run_options,
        const struct vaccel_tf_node *in_nodes, const struct vaccel_tf_tensor *in, int nr_inputs,
        const struct vaccel_tf_node *out_nodes, struct vaccel_tf_tensor *out, int nr_outputs,
        struct vaccel_tf_status *status)
{
	vaccel_debug("TensorFlow: run graph");

	if (!session) {
		vaccel_debug("Invalid session");
		return VACCEL_EINVAL;
	}

	// Get implementation
	int (*plugin_op)() = get_plugin_op(VACCEL_TF_MODEL_RUN_GRAPH);
	if (!plugin_op)
		return VACCEL_ENOTSUP;

	return plugin_op(session, model, run_options, in_nodes, in, nr_inputs,
			out_nodes, out, nr_outputs, status);
}
