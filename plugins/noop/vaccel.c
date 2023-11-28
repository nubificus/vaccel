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

#include "ops/vaccel_ops.h"
#include <stdio.h>
#include <string.h>

#include <vaccel.h>
#include <ops/torch.h>

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
	long long int m, long long int n, long long int k,
	float alpha,
	float *a, long long int lda,
	float *b, long long int ldb,
	float beta,
	float *c, long long int ldc
) {
	fprintf(stdout, "[noop] Calling sgemm for session %u\n",
		sess->session_id);

	fprintf(stdout, "[noop] Dumping arguments for sgemm:\n");
	fprintf(stdout, "[noop] m: %lld n: %lld k: %lld\n", m, n, k);
	fprintf(stdout, "[noop] alpha: %f\n", alpha);
	fprintf(stdout, "[noop] A: %p lda: %lld\n", a, lda);
	fprintf(stdout, "[noop] B: %p ldb: %lld\n", b, ldb);
	fprintf(stdout, "[noop] beta: %f\n", beta);
	fprintf(stdout, "[noop] C: %p ldc: %lld\n", c, ldc);
	
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

	fprintf(stdout, "[noop] Dumping arguments for minmax: ndata:%d\n", ndata);
	fprintf(stdout, "[noop] low: %d high: %d \n", low_threshold, high_threshold);


        //*outdata = tmp_min;
	memcpy(outdata, indata, ndata * sizeof(double));
        *max = tmp_max;
        *min = tmp_min;

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
	fprintf(stdout, "[noop] len_img: %zu\n", len_img);
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
	fprintf(stdout, "[noop] len_img: %zu\n", len_img);

	return VACCEL_OK;
}

static int noop_img_segme(struct vaccel_session *sess, const void *img,
			  const unsigned char *out_imgname, size_t len_img,
			  size_t len_out_imgname)
{
	fprintf(stdout, "[noop] Calling Image segmentation for session %u\n",
		sess->session_id);

	fprintf(stdout, "[noop] Dumping arguments for Image segmentation:\n");
	fprintf(stdout, "[noop] len_img: %zu\n", len_img);

	return VACCEL_OK;
}

static int noop_img_pose(struct vaccel_session *sess, const void *img,
			  const unsigned char *out_imgname, size_t len_img,
			  size_t len_out_imgname)
{
	fprintf(stdout, "[noop] Calling Image pose for session %u\n",
		sess->session_id);

	fprintf(stdout, "[noop] Dumping arguments for Image pose:\n");
	fprintf(stdout, "[noop] len_img: %zu\n", len_img);

	return VACCEL_OK;
}

static int noop_img_depth(struct vaccel_session *sess, const void *img,
			  const unsigned char *out_imgname, size_t len_img,
			  size_t len_out_imgname)
{
	fprintf(stdout, "[noop] Calling Image depth for session %u\n",
		sess->session_id);

	fprintf(stdout, "[noop] Dumping arguments for Image depth:\n");
	fprintf(stdout, "[noop] len_img: %zu\n", len_img);

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
	fprintf(stdout, "[noop] nr_read: %zu nr_write: %zu\n", nr_read, nr_write);

	return VACCEL_OK;
}

static int noop_exec_with_resource(struct vaccel_session *sess, struct vaccel_shared_object *object,
		     const char *fn_symbol, struct vaccel_arg *read,
		     size_t nr_read, struct vaccel_arg *write, size_t nr_write)
{
	if (!sess) {
		noop_error("Invalid session\n");
		return VACCEL_EINVAL;
	}

	if (!object) {
		noop_error("Invalid shared object\n");
		return VACCEL_EINVAL;
	}

	if (!vaccel_sess_has_resource(sess, object->resource)) {
		noop_error("Shared object is not registered with session\n");
		return VACCEL_ENOENT;
	}

	fprintf(stdout, "[noop] Calling exec_with_resource for session %u\n",
		sess->session_id);

	const char* library = object->file.path;
	fprintf(stdout, "[noop] object file path: %s\n", object->file.path);
	fprintf(stdout, "[noop] Dumping arguments for exec_with_resource:\n");
	fprintf(stdout, "[noop] library: %s symbol: %s\n", library, fn_symbol);
	fprintf(stdout, "[noop] nr_read: %zu nr_write: %zu\n", nr_read, nr_write);
	
	if (nr_write > 0) {
		sprintf(write[0].buf, "I got this input: %d\n", *(int*)read[0].buf);
	}

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
		noop_info("Run options -> %p, %zu\n", run_options->data,
			run_options->size);

	noop_info("Number of inputs: %d\n", nr_inputs);
	for (int i = 0; i < nr_inputs; ++i) {
		noop_info("\tNode %d: %s:%lld\n", i, in_nodes[i].name,
				in_nodes[i].id);
		noop_info("\t#dims: %d -> {", in[i]->nr_dims);
		for (int j = 0; j < in[i]->nr_dims; ++j)
			printf("%d%s", in[i]->dims[j],
				(j == in[i]->nr_dims - 1) ? "}\n" : " ");

		noop_info("\tData type: %d\n", in[i]->data_type);
		noop_info("\tData -> %p, %zu\n", in[i]->data, in[i]->size);
	}

	noop_info("Number of outputs: %d\n", nr_outputs);
	for (int i = 0; i < nr_outputs; ++i) {
		noop_info("\tNode %d: %s:%lld\n", i, out_nodes[i].name,
				out_nodes[i].id);
		out[i] = malloc(sizeof(*out[i]));
		out[i]->nr_dims = in[i]->nr_dims;
		out[i]->dims = malloc(sizeof(*out[i]->dims) * in[i]->nr_dims);
		memcpy(out[i]->dims,in[i]->dims, sizeof(*in[i]->dims) * in[i]->nr_dims);
		out[i]->data_type = in[i]->data_type;
		out[i]->data = malloc(in[i]->size * sizeof(double));
		memcpy(out[i]->data, in[i]->data, in[i]->size);
		out[i]->size = in[i]->size;
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

static int v_arraycopy(struct vaccel_session *session, int *a, int *b, size_t c)
{
	int i = 0;
	// fprintf(stdout, "[noop] Calling v_arraycopy for session %u\n",
	// 	session->session_id);

	// fprintf(stdout, "[noop] Dumping arguments for v_arracycopy:\n");
	// fprintf(stdout, "[noop] size: %zu \n", c);

	/* Fill output with dummy values */
	for (i = 0; i < c ; i++) {
		b[i] = a[i];
	}


	return VACCEL_OK;
}

static int v_vectoradd(struct vaccel_session *session, float *a, float *b,
				                       float *c, size_t len_a, size_t len_b)
{
	int i = 0;
	// fprintf(stdout, "[noop] Calling v_vectoradd for session %u\n",
	// 	session->session_id);

	// fprintf(stdout, "[noop] Dumping arguments for v_vectoradd:\n");
	// fprintf(stdout, "[noop] len_a: %zu len_b: %zu \n", len_a, len_b);

	/* Fill output with dummy values */
	for (i = 0; i < len_a ; i++) {
		c[i] = a[i] + b[i];
	}


	return VACCEL_OK;
}

static int v_parallel(struct vaccel_session *session, float *a, float *b,
				                      float *add_out, float *mult_out, size_t len_a)
{
	int i = 0;
	// fprintf(stdout, "[noop] Calling v_parallel for session %u\n",
	// 	session->session_id);

	// fprintf(stdout, "[noop] Dumping arguments for v_parallel:\n");
	// fprintf(stdout, "[noop] len_a: %zu\n", len_a);

	/* Fill output with dummy values */
	for (i = 0; i < len_a ; i++) {
		add_out[i] = a[i] + b[i];
		mult_out[i] = 1;
	}

	return VACCEL_OK;
}

static int v_mmult(struct vaccel_session *session, float *a, float *b,
				                   float *c_out, size_t len_a)
{
	int i = 0;
	// fprintf(stdout, "[noop] Calling v_mmult for session %u\n",
	// 	session->session_id);

	// fprintf(stdout, "[noop] Dumping arguments for v_mmult:\n");
	// fprintf(stdout, "[noop] len_a: %zu\n", len_a);

	/* Fill output with dummy values */
	for (i = 0; i < len_a ; i++) {
		c_out[i] = 9.1;
	}
	return VACCEL_OK;
}


// Torch NOOP test
static int noop_torch_jitload_forward(struct vaccel_session *session, 
		const struct vaccel_torch_saved_model *model,
		const struct vaccel_torch_buffer *run_options,
		struct vaccel_torch_tensor **in_tensor,
		int nr_read,
		/*char **tags, */
		struct vaccel_torch_tensor **out_tensor,
		int nr_write)
{
	if(!session) {
		noop_error("Invalid session \n");
		return VACCEL_EINVAL;
	}

	if(!model) {
		noop_error("Invalid model path \n");
		return VACCEL_EINVAL;
	}

	fprintf(stdout, "[noop] Calling jitload_forward for session %u\n",
					session->session_id);

	noop_info("Number of inputs: %d\n", nr_read);
	for (int i = 0; i < nr_read; ++i) {
		noop_info("\t#dims: %d -> {", in_tensor[i]->nr_dims);
		for (int j = 0; j < in_tensor[i]->nr_dims; ++j)
			printf("%d%s", in_tensor[i]->dims[j],
				(j == in_tensor[i]->nr_dims - 1) ? "}\n" : " ");

		noop_info("\tData type: %d\n", in_tensor[i]->data_type);
		noop_info("\tData -> %p, %zu\n", in_tensor[i]->data, in_tensor[i]->size);
	}

	noop_info("Number of outputs: %d\n", nr_write);
	for (int i = 0; i < nr_write; ++i) {
		out_tensor[i] = malloc(sizeof(*out_tensor[i]));
		out_tensor[i]->nr_dims = in_tensor[i]->nr_dims;
		out_tensor[i]->dims = malloc(sizeof(*out_tensor[i]->dims) * in_tensor[i]->nr_dims);
		memcpy(out_tensor[i]->dims,in_tensor[i]->dims, sizeof(*in_tensor[i]->dims) * in_tensor[i]->nr_dims);
		out_tensor[i]->data_type = in_tensor[i]->data_type;
		out_tensor[i]->data = malloc(in_tensor[i]->size * sizeof(double));
		memcpy(out_tensor[i]->data, in_tensor[i]->data, in_tensor[i]->size);
		out_tensor[i]->size = in_tensor[i]->size;
	}


	return VACCEL_OK;
}

static int noop_torch_sgemm(struct vaccel_session *session,
		struct vaccel_torch_tensor **in_A,
		struct vaccel_torch_tensor **in_B,
		struct vaccel_torch_tensor **in_C,
		int M, int N, int K,
		struct vaccel_torch_tensor **out)
{
	if(!session) {
		noop_error("Invalid session \n");
		return VACCEL_EINVAL;
	}

	fprintf(stdout, "[noop] Calling torch_sgemm for session %u\n",
					session->session_id);
	fprintf(stdout, "[noop] Dumping arguments for torch_sgemm:\n");
	fprintf(stdout, "[noop] m: %d n: %d k: %d\n", M, N, K);
	return VACCEL_OK;
}

static int noop_opencv(struct vaccel_session *sess, struct vaccel_arg *read,
		     size_t nr_read, struct vaccel_arg *write, size_t nr_write)
{
	int i = 0;
	fprintf(stdout, "[noop] Calling opencv for session %u\n",
		sess->session_id);

	fprintf(stdout, "[noop] Dumping arguments for opencv:\n");
	fprintf(stdout, "[noop] nr_read: %zu nr_write: %zu\n", nr_read, nr_write);
	fprintf(stdout, "[noop] [OpenCV] function: %u\n", *(uint8_t*)read[0].buf);
	for (i = 1 ; i < nr_read; i++) {
		fprintf(stdout, "[noop] opencv read[%d] size: %u\n", i, read[i].size);
	}
	for (i = 0 ; i < nr_write; i++) {
		fprintf(stdout, "[noop] opencv write[%d] size: %u\n", i, write[i].size);
		size_t *header;
		header = write[i].buf;
		header[0] = write[i].size/sizeof(float);
	}
#if 0
	uint8_t *p;
	p = write[1].buf;
	p[10] = 1;
#endif

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
	VACCEL_OP_INIT(ops[12], VACCEL_F_ARRAYCOPY, v_arraycopy),
	VACCEL_OP_INIT(ops[13], VACCEL_F_VECTORADD, v_vectoradd),
	VACCEL_OP_INIT(ops[14], VACCEL_F_PARALLEL, v_parallel),
	VACCEL_OP_INIT(ops[15], VACCEL_F_MMULT, v_mmult),
	VACCEL_OP_INIT(ops[16], VACCEL_EXEC_WITH_RESOURCE, noop_exec_with_resource),
	VACCEL_OP_INIT(ops[17], VACCEL_TORCH_JITLOAD_FORWARD, noop_torch_jitload_forward),
	VACCEL_OP_INIT(ops[18], VACCEL_TORCH_SGEMM, noop_torch_sgemm),
	VACCEL_OP_INIT(ops[19], VACCEL_OPENCV, noop_opencv),
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
	.version = VACCELRT_VERSION,
	.type = VACCEL_PLUGIN_DEBUG,
	.init = init,
	.fini = fini
)
