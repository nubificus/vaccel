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

#include <stdio.h>
#include <string.h>

#include <vaccel.h>

#define noop_info(fmt, ...) \
	fprintf(stdout, "[noop] " fmt, ##__VA_ARGS__)

#define noop_error(fmt, ...) \
	fprintf(stderr, "[noop] " fmt, ##__VA_ARGS__)

static int noop_noop(struct vaccel_session *sess)
{
	fprintf(stdout, "[noop] Calling no-op for session %u\n",
		sess->session_id);

	return VACCEL_OK;
}

static int noop_sgemm(
	struct vaccel_session *sess,
	int64_t m, int64_t n, int64_t k,
	float alpha,
	float *a, int64_t lda,
	float *b, int64_t ldb,
	float beta,
	float *c, int64_t ldc
) {
	fprintf(stdout, "[noop] Calling sgemm for session %u\n",
		sess->session_id);

	fprintf(stdout, "[noop] Dumping arguments for sgemm:\n");
	fprintf(stdout, "[noop] m: %ld n: %ld k: %ld\n", m, n, k);
	fprintf(stdout, "[noop] alpha: %f\n", alpha);
	fprintf(stdout, "[noop] A: %p lda: %ld\n", a, lda);
	fprintf(stdout, "[noop] B: %p ldb: %ld\n", b, ldb);
	fprintf(stdout, "[noop] beta: %f\n", beta);
	fprintf(stdout, "[noop] C: %p ldc: %ld\n", c, ldc);
	
	return VACCEL_OK;
}

int noop_minmax(
        struct vaccel_session *sess,
        const double *indata, int ndata,
        int low_threshold, int high_threshold,
        double *outdata,
        double *min,
        double *max
) {
        double tmp_max = -1.0;
        double tmp_min = 10000.0;

        if (!sess)
                return VACCEL_EINVAL;

	fprintf(stdout, "[noop] Calling minmax for session %u\n",
		sess->session_id);

	fprintf(stdout, "[noop] Dumping arguments for minmax :\n");
	fprintf(stdout, "[noop] low: %d high: %d \n", low_threshold, high_threshold);


        *min = tmp_min;
        *max = tmp_max;

        return VACCEL_OK;
}

static int noop_img_class(struct vaccel_session *sess, const void *img,
			  unsigned char *out_text, unsigned char *out_imgname,
			  size_t len_img, size_t len_out_text,
			  size_t len_out_imgname)
{
	fprintf(stdout, "[noop] Calling Image classification for session %u\n",
		sess->session_id);

	fprintf(stdout, "[noop] Dumping arguments for Image classification:\n");
	fprintf(stdout, "[noop] len_img: %lu\n", len_img);
	fprintf(stdout, "[noop] will return a dummy result\n");
	sprintf(out_text, "This is a dummy classification tag!");
	len_out_text = strlen("This is a dummy classification tag!");

	return VACCEL_OK;
}

static int noop_img_detect(struct vaccel_session *sess, const void *img,
			   const unsigned char *out_imgname, size_t len_img,
			   size_t len_out_imgname)
{
	fprintf(stdout, "[noop] Calling Image detection for session %u\n",
		sess->session_id);

	fprintf(stdout, "[noop] Dumping arguments for Image detection:\n");
	fprintf(stdout, "[noop] len_img: %lu\n", len_img);

	return VACCEL_OK;
}

static int noop_img_segme(struct vaccel_session *sess, const void *img,
			  const unsigned char *out_imgname, size_t len_img,
			  size_t len_out_imgname)
{
	fprintf(stdout, "[noop] Calling Image segmentation for session %u\n",
		sess->session_id);

	fprintf(stdout, "[noop] Dumping arguments for Image segmentation:\n");
	fprintf(stdout, "[noop] len_img: %lu\n", len_img);

	return VACCEL_OK;
}

static int noop_img_pose(struct vaccel_session *sess, const void *img,
			  const unsigned char *out_imgname, size_t len_img,
			  size_t len_out_imgname)
{
	fprintf(stdout, "[noop] Calling Image pose for session %u\n",
		sess->session_id);

	fprintf(stdout, "[noop] Dumping arguments for Image pose:\n");
	fprintf(stdout, "[noop] len_img: %lu\n", len_img);

	return VACCEL_OK;
}

static int noop_img_depth(struct vaccel_session *sess, const void *img,
			  const unsigned char *out_imgname, size_t len_img,
			  size_t len_out_imgname)
{
	fprintf(stdout, "[noop] Calling Image depth for session %u\n",
		sess->session_id);

	fprintf(stdout, "[noop] Dumping arguments for Image depth:\n");
	fprintf(stdout, "[noop] len_img: %lu\n", len_img);

	return VACCEL_OK;
}

static int noop_exec(struct vaccel_session *sess, const char *library,
		     const char *fn_symbol, struct vaccel_arg *read,
		     size_t nr_read, struct vaccel_arg *write, size_t nr_write)
{
	fprintf(stdout, "[noop] Calling exec for session %u\n",
		sess->session_id);

	fprintf(stdout, "[noop] Dumping arguments for exec:\n");
	fprintf(stdout, "[noop] library: %s symbol: %s\n", library, fn_symbol);
	fprintf(stdout, "[noop] nr_read: %lu nr_write: %lu\n", nr_read, nr_write);

	return VACCEL_OK;
}

static int noop_tf_session_load(
	struct vaccel_session *session,
	struct vaccel_tf_saved_model *model,
	struct vaccel_tf_status *status)
{
	if (!session) {
		noop_error("Invalid session\n");
		return VACCEL_EINVAL;
	}

	if (!model) {
		noop_error("Invalid TensorFlow model\n");
		return VACCEL_EINVAL;
	}

	if (!vaccel_sess_has_resource(session, model->resource)) {
		noop_error("Model is not registered with session\n");
		return VACCEL_ENOENT;
	}

	if (status) {
		status->message = strdup("Operation handled by noop plugin");
		if (!status->message)
			return VACCEL_ENOMEM;
	}

	return VACCEL_OK;
}

static int noop_tf_session_run(
	struct vaccel_session *session,
        const struct vaccel_tf_saved_model *model, const struct vaccel_tf_buffer *run_options,
        const struct vaccel_tf_node *in_nodes, struct vaccel_tf_tensor *const *in, int nr_inputs,
        const struct vaccel_tf_node *out_nodes, struct vaccel_tf_tensor **out, int nr_outputs,
        struct vaccel_tf_status *status)
{
	if (!session) {
		noop_error("Invalid session\n");
		return VACCEL_EINVAL;
	}

	if (!model) {
		noop_error("Invalid TensorFlow model\n");
		return VACCEL_EINVAL;
	}

	if (run_options)
		noop_info("Run options -> %p, %lu\n", run_options->data,
			run_options->size);

	noop_info("Number of inputs: %d\n", nr_inputs);
	for (int i = 0; i < nr_inputs; ++i) {
		noop_info("\tNode %d: %s:%ld\n", i, in_nodes[i].name,
				in_nodes[i].id);
		noop_info("\t#dims: %d -> {", in[i]->nr_dims);
		for (int j = 0; j < in[i]->nr_dims; ++j)
			printf("%ld%s", in[i]->dims[j],
				(j == in[i]->nr_dims - 1) ? "}\n" : " ");

		noop_info("\tData type: %d\n", in[i]->data_type);
		noop_info("\tData -> %p, %lu\n", in[i]->data, in[i]->size);
	}

	noop_info("Number of outputs: %d\n", nr_outputs);
	for (int i = 0; i < nr_outputs; ++i) {
		noop_info("\tNode %d: %s:%ld\n", i, out_nodes[i].name,
				out_nodes[i].id);
		out[i] = malloc(sizeof(*out[i]));
		out[i]->nr_dims = 0;
		out[i]->dims = NULL;
		out[i]->data_type = 0;
		out[i]->data = NULL;
		out[i]->size = 0;
	}

	if (status) {
		status->message = strdup("Operation handled by noop plugin");
		if (!status->message)
			return VACCEL_ENOMEM;
	}

	return VACCEL_OK;
}

static int noop_tf_session_delete(
	struct vaccel_session *session,
        const struct vaccel_tf_saved_model *model,
	struct vaccel_tf_status *status
) {
	if (!session) {
		noop_error("Invalid session\n");
		return VACCEL_EINVAL;
	}

	if (!model) {
		noop_error("Invalid TensorFlow model\n");
		return VACCEL_EINVAL;
	}

	if (status) {
		status->message = strdup("Operation handled by noop plugin");
		if (!status->message)
			return VACCEL_ENOMEM;
	}

	return VACCEL_OK;
}

struct vaccel_op ops[] = {
	VACCEL_OP_INIT(ops[0], VACCEL_NO_OP, noop_noop),
	VACCEL_OP_INIT(ops[1], VACCEL_BLAS_SGEMM, noop_sgemm),
	VACCEL_OP_INIT(ops[2], VACCEL_IMG_CLASS, noop_img_class),
	VACCEL_OP_INIT(ops[3], VACCEL_IMG_DETEC, noop_img_detect),
	VACCEL_OP_INIT(ops[4], VACCEL_IMG_SEGME, noop_img_segme),
	VACCEL_OP_INIT(ops[5], VACCEL_IMG_POSE, noop_img_pose),
	VACCEL_OP_INIT(ops[6], VACCEL_IMG_DEPTH, noop_img_depth),
	VACCEL_OP_INIT(ops[7], VACCEL_EXEC, noop_exec),
	VACCEL_OP_INIT(ops[8], VACCEL_TF_SESSION_LOAD, noop_tf_session_load),
	VACCEL_OP_INIT(ops[9], VACCEL_TF_SESSION_RUN, noop_tf_session_run),
	VACCEL_OP_INIT(ops[10], VACCEL_TF_SESSION_DELETE, noop_tf_session_delete),
	VACCEL_OP_INIT(ops[11], VACCEL_MINMAX, noop_minmax),
};

static int init(void)
{
	return register_plugin_functions(ops, sizeof(ops) / sizeof(ops[0]));
}

static int fini(void)
{
	return VACCEL_OK;
}

VACCEL_MODULE(
	.name = "noop",
	.version = "0.1",
	.init = init,
	.fini = fini
)
