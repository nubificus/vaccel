// SPDX-License-Identifier: Apache-2.0

#define _POSIX_C_SOURCE 200809L

#include "vaccel.h"
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define noop_debug(fmt, ...) vaccel_debug("[noop] " fmt, ##__VA_ARGS__)
#define noop_error(fmt, ...) vaccel_error("[noop] " fmt, ##__VA_ARGS__)

static int noop_noop(struct vaccel_session *sess)
{
	noop_debug("Calling no-op for session %" PRId64 "", sess->id);

	return VACCEL_OK;
}

static void exec_gen_dummy_output(struct vaccel_arg *read,
				  struct vaccel_arg *write)
{
	size_t nr_output = write[0].size / sizeof(int);
	if (nr_output >= 1) {
		noop_debug("will return dummy output = input");
		int *input = (int *)read[0].buf;
		int *output = (int *)write[0].buf;
		for (size_t j = 0; j < nr_output; j++) {
			output[j] = input[j];
		}
	}
}

static int noop_exec(struct vaccel_session *sess, const char *library,
		     const char *fn_symbol, struct vaccel_arg *read,
		     size_t nr_read, struct vaccel_arg *write, size_t nr_write)
{
	if (!sess || !library || !fn_symbol) {
		noop_error("Invalid input arguments");
		return VACCEL_EINVAL;
	}

	noop_debug("Calling exec for session %" PRId64 "", sess->id);

	noop_debug("Dumping arguments for exec:");
	noop_debug("library: %s symbol: %s", library, fn_symbol);
	noop_debug("nr_read: %zu nr_write: %zu", nr_read, nr_write);

	if ((nr_write == 1) && (write[0].size == read[0].size) &&
	    (write[0].size % sizeof(int) == 0))
		exec_gen_dummy_output(read, write);

	return VACCEL_OK;
}

static int noop_exec_with_resource(struct vaccel_session *sess,
				   struct vaccel_resource *object,
				   const char *fn_symbol,
				   struct vaccel_arg *read, size_t nr_read,
				   struct vaccel_arg *write, size_t nr_write)
{
	if (!sess) {
		noop_error("Invalid session");
		return VACCEL_EINVAL;
	}

	if (!object) {
		noop_error("Invalid shared object");
		return VACCEL_EINVAL;
	}

	noop_debug("Calling exec_with_resource for session %" PRId64 "",
		   sess->id);

	noop_debug("Dumping arguments for exec_with_resource:");

	size_t nr_deps = object->nr_blobs - 1;
	for (size_t i = 0; i < nr_deps; i++) {
		char *dep_library = object->blobs[i]->path;
		if (dep_library == NULL) {
			vaccel_error("Could not get path of file %zu", i);
			return VACCEL_EINVAL;
		}
		noop_debug("dep library: %s", dep_library);
	}

	char *library = object->blobs[nr_deps]->path;
	if (library == NULL) {
		vaccel_error("Could not get path of file %zu", nr_deps);
		return VACCEL_EINVAL;
	}

	noop_debug("library: %s symbol: %s", library, fn_symbol);
	noop_debug("nr_read: %zu nr_write: %zu", nr_read, nr_write);

	if ((nr_write == 1) && (write[0].size == read[0].size) &&
	    (write[0].size % sizeof(int) == 0))
		exec_gen_dummy_output(read, write);

	return VACCEL_OK;
}

static int noop_image_classify(struct vaccel_session *sess, const void *img,
			       unsigned char *out_text,
			       unsigned char *out_imgname, size_t len_img,
			       size_t len_out_text, size_t len_out_imgname)
{
	if (!sess || !img) {
		noop_error("Invalid input arguments");
		return VACCEL_EINVAL;
	}
	int ret;

	noop_debug("Calling Image classification for session %" PRId64 "",
		   sess->id);

	struct vaccel_resource *model = NULL;
	ret = vaccel_session_resource_by_type(sess, &model,
					      VACCEL_RESOURCE_MODEL);
	if (ret && ret != VACCEL_ENOENT) {
		noop_error("Failed to get model");
		return ret;
	}

	noop_debug("Dumping arguments for Image classification:");
	noop_debug("model: %s", (model) ? model->blobs[0]->path : NULL);
	noop_debug("len_img: %zu", len_img);
	noop_debug("len_out_text: %zu", len_out_text);
	if (len_out_imgname)
		noop_debug("len_out_imgname: %zu", len_out_imgname);
	if (out_text) {
		noop_debug("will return a dummy result");
		ret = snprintf((char *)out_text, len_out_text,
			       "This is a dummy classification tag!");
		if (ret <= 0)
			return VACCEL_EINVAL;
	}
	if (out_imgname) {
		noop_debug("will return a dummy result");
		ret = snprintf((char *)out_imgname, len_out_imgname,
			       "This is a dummy imgname!");
		if (ret <= 0)
			return VACCEL_EINVAL;
	}

	return VACCEL_OK;
}

static int noop_image_detect(struct vaccel_session *sess, const void *img,
			     const unsigned char *out_imgname, size_t len_img,
			     size_t len_out_imgname)
{
	if (!sess || !img) {
		noop_error("Invalid input arguments");
		return VACCEL_EINVAL;
	}
	int ret;

	noop_debug("Calling Image detection for session %" PRId64 "", sess->id);

	struct vaccel_resource *model = NULL;
	ret = vaccel_session_resource_by_type(sess, &model,
					      VACCEL_RESOURCE_MODEL);
	if (ret && ret != VACCEL_ENOENT) {
		noop_error("Failed to get model");
		return ret;
	}

	noop_debug("Dumping arguments for Image detection:");
	noop_debug("model: %s", (model) ? model->blobs[0]->path : NULL);
	noop_debug("len_img: %zu", len_img);
	if (len_out_imgname)
		noop_debug("len_out_imgname: %zu", len_out_imgname);
	if (out_imgname) {
		noop_debug("will return a dummy result");
		ret = snprintf((char *)out_imgname, len_out_imgname,
			       "This is a dummy imgname!");
		if (ret <= 0)
			return VACCEL_EINVAL;
	}

	return VACCEL_OK;
}

static int noop_image_segment(struct vaccel_session *sess, const void *img,
			      const unsigned char *out_imgname, size_t len_img,
			      size_t len_out_imgname)
{
	if (!sess || !img) {
		noop_error("Invalid input arguments");
		return VACCEL_EINVAL;
	}
	int ret;

	noop_debug("Calling Image segmentation for session %" PRId64 "",
		   sess->id);

	struct vaccel_resource *model = NULL;
	ret = vaccel_session_resource_by_type(sess, &model,
					      VACCEL_RESOURCE_MODEL);
	if (ret && ret != VACCEL_ENOENT) {
		noop_error("Failed to get model");
		return ret;
	}

	noop_debug("Dumping arguments for Image segmentation:");
	noop_debug("model: %s", (model) ? model->blobs[0]->path : NULL);
	noop_debug("len_img: %zu", len_img);
	if (len_out_imgname)
		noop_debug("len_out_imgname: %zu", len_out_imgname);
	if (out_imgname) {
		noop_debug("will return a dummy result");
		ret = snprintf((char *)out_imgname, len_out_imgname,
			       "This is a dummy imgname!");
		if (ret <= 0)
			return VACCEL_EINVAL;
	}

	return VACCEL_OK;
}

static int noop_image_pose(struct vaccel_session *sess, const void *img,
			   const unsigned char *out_imgname, size_t len_img,
			   size_t len_out_imgname)
{
	if (!sess || !img) {
		noop_error("Invalid input arguments");
		return VACCEL_EINVAL;
	}
	int ret;

	noop_debug("Calling Image pose for session %" PRId64 "", sess->id);

	struct vaccel_resource *model = NULL;
	ret = vaccel_session_resource_by_type(sess, &model,
					      VACCEL_RESOURCE_MODEL);
	if (ret && ret != VACCEL_ENOENT) {
		noop_error("Failed to get model");
		return ret;
	}

	noop_debug("Dumping arguments for Image pose:");
	noop_debug("model: %s", (model) ? model->blobs[0]->path : NULL);
	noop_debug("len_img: %zu", len_img);
	if (len_out_imgname)
		noop_debug("len_out_imgname: %zu", len_out_imgname);
	if (out_imgname) {
		noop_debug("will return a dummy result");
		ret = snprintf((char *)out_imgname, len_out_imgname,
			       "This is a dummy imgname!");
		if (ret <= 0)
			return VACCEL_EINVAL;
	}

	return VACCEL_OK;
}

static int noop_image_depth(struct vaccel_session *sess, const void *img,
			    const unsigned char *out_imgname, size_t len_img,
			    size_t len_out_imgname)
{
	if (!sess || !img) {
		noop_error("Invalid input arguments");
		return VACCEL_EINVAL;
	}
	int ret;

	noop_debug("Calling Image depth for session %" PRId64 "", sess->id);

	struct vaccel_resource *model = NULL;
	ret = vaccel_session_resource_by_type(sess, &model,
					      VACCEL_RESOURCE_MODEL);
	if (ret && ret != VACCEL_ENOENT) {
		noop_error("Failed to get model");
		return ret;
	}

	noop_debug("Dumping arguments for Image depth:");
	noop_debug("model: %s", (model) ? model->blobs[0]->path : NULL);
	noop_debug("len_img: %zu", len_img);
	if (len_out_imgname)
		noop_debug("len_out_imgname: %zu", len_out_imgname);
	if (out_imgname) {
		noop_debug("will return a dummy result");
		ret = snprintf((char *)out_imgname, len_out_imgname,
			       "This is a dummy imgname!");
		if (ret <= 0)
			return VACCEL_EINVAL;
	}

	return VACCEL_OK;
}

static int noop_tf_model_load(struct vaccel_session *sess,
			      struct vaccel_resource *model,
			      struct vaccel_tf_status *status)
{
	if (!sess) {
		noop_error("Invalid session");
		return VACCEL_EINVAL;
	}

	if (!model) {
		noop_error("Invalid TensorFlow model");
		return VACCEL_EINVAL;
	}

	noop_debug("Calling tf_model_load for session %" PRId64 "", sess->id);

	if (status) {
		status->error_code = 0;
		status->message = strdup("Operation handled by noop plugin");
		if (!status->message)
			return VACCEL_ENOMEM;
	}

	return VACCEL_OK;
}

static int noop_tf_model_unload(struct vaccel_session *sess,
				const struct vaccel_resource *model,
				struct vaccel_tf_status *status)
{
	if (!sess) {
		noop_error("Invalid session");
		return VACCEL_EINVAL;
	}

	if (!model) {
		noop_error("Invalid TensorFlow model");
		return VACCEL_EINVAL;
	}

	noop_debug("Calling tf_model_unload for session %" PRId64 "", sess->id);

	if (status) {
		status->error_code = 0;
		status->message = strdup("Operation handled by noop plugin");
		if (!status->message)
			return VACCEL_ENOMEM;
	}

	return VACCEL_OK;
}

static int noop_tf_model_run(struct vaccel_session *sess,
			     const struct vaccel_resource *model,
			     const struct vaccel_tf_buffer *run_options,
			     const struct vaccel_tf_node *in_nodes,
			     struct vaccel_tf_tensor *const *in, int nr_inputs,
			     const struct vaccel_tf_node *out_nodes,
			     struct vaccel_tf_tensor **out, int nr_outputs,
			     struct vaccel_tf_status *status)
{
	if (!sess) {
		noop_error("Invalid session");
		return VACCEL_EINVAL;
	}

	if (!model) {
		noop_error("Invalid TensorFlow model");
		return VACCEL_EINVAL;
	}

	noop_debug("Calling tf_model_run for session %" PRId64 "", sess->id);

	if (run_options)
		noop_debug("Run options -> %p, %zu", run_options->data,
			   run_options->size);

	noop_debug("Number of inputs: %d", nr_inputs);
	for (int i = 0; i < nr_inputs; ++i) {
		noop_debug("\tNode %d: %s:%d", i, in_nodes[i].name,
			   in_nodes[i].id);
		noop_debug("\t#dims: %d -> {", in[i]->nr_dims);
		for (int j = 0; j < in[i]->nr_dims; ++j)
			noop_debug("\t\t%" PRId64, in[i]->dims[j]);
		noop_debug("\t}");

		noop_debug("\tData type: %d", in[i]->data_type);
		noop_debug("\tData -> %p, %zu", in[i]->data, in[i]->size);
	}

	noop_debug("Number of outputs: %d", nr_outputs);
	for (int i = 0; i < nr_outputs; ++i) {
		out[i] = NULL;
	}
	for (int i = 0; i < nr_outputs; ++i) {
		noop_debug("\tNode %d: %s:%d", i, out_nodes[i].name,
			   out_nodes[i].id);
		out[i] = malloc(sizeof(*out[i]));
		if (!out[i])
			goto free_tf;
		out[i]->nr_dims = in[i]->nr_dims;
		out[i]->data_type = in[i]->data_type;
		out[i]->owned = 1;
		out[i]->size = in[i]->size;
		out[i]->dims = malloc(sizeof(*out[i]->dims) * in[i]->nr_dims);
		out[i]->data = malloc(in[i]->size * sizeof(double));
		if (!out[i]->dims || !out[i]->data)
			goto free_tf;
		memcpy(out[i]->dims, in[i]->dims,
		       sizeof(*in[i]->dims) * in[i]->nr_dims);
		memcpy(out[i]->data, in[i]->data, in[i]->size);
	}

	if (status) {
		status->error_code = 0;
		status->message = strdup("Operation handled by noop plugin");
		if (!status->message)
			goto free_tf;
	}

	return VACCEL_OK;

free_tf:
	for (int i = 0; i < nr_outputs; ++i) {
		if (!out[i])
			continue;

		if (out[i]->dims)
			free(out[i]->dims);
		if (out[i]->data)
			free(out[i]->data);

		free(out[i]);
	}
	return VACCEL_ENOMEM;
}

static int noop_tflite_model_load(struct vaccel_session *sess,
				  struct vaccel_resource *model)
{
	if (!sess) {
		noop_error("Invalid session");
		return VACCEL_EINVAL;
	}

	if (!model) {
		noop_error("Invalid model");
		return VACCEL_EINVAL;
	}

	noop_debug("Calling tflite_model_load for session %" PRId64 "",
		   sess->id);

	return VACCEL_OK;
}

static int noop_tflite_model_unload(struct vaccel_session *sess,
				    const struct vaccel_resource *model)
{
	if (!sess) {
		noop_error("Invalid session");
		return VACCEL_EINVAL;
	}

	if (!model) {
		noop_error("Invalid model");
		return VACCEL_EINVAL;
	}

	noop_debug("Calling tflite_model_unload for session %" PRId64 "",
		   sess->id);

	return VACCEL_OK;
}

static int noop_tflite_model_run(struct vaccel_session *sess,
				 const struct vaccel_resource *model,
				 struct vaccel_tflite_tensor *const *in,
				 int nr_inputs,
				 struct vaccel_tflite_tensor **out,
				 int nr_outputs, uint8_t *status)
{
	if (!sess) {
		noop_error("Invalid session");
		return VACCEL_EINVAL;
	}

	if (!model) {
		noop_error("Invalid model");
		return VACCEL_EINVAL;
	}

	noop_debug("Calling tflite_model_run for session %" PRId64 "",
		   sess->id);

	noop_debug("Number of inputs: %d", nr_inputs);
	for (int i = 0; i < nr_inputs; ++i) {
		noop_debug("\t#dims: %d -> {", in[i]->nr_dims);
		for (int j = 0; j < in[i]->nr_dims; ++j)
			noop_debug("\t\t%" PRId32, in[i]->dims[j]);
		noop_debug("\t}");

		noop_debug("\tData type: %d", in[i]->data_type);
		noop_debug("\tData -> %p, %zu", in[i]->data, in[i]->size);
	}

	noop_debug("Number of outputs: %d", nr_outputs);
	for (int i = 0; i < nr_outputs; ++i) {
		out[i] = NULL;
	}
	for (int i = 0; i < nr_outputs; ++i) {
		out[i] = malloc(sizeof(*out[i]));
		if (!out[i])
			goto free_tflite;
		out[i]->nr_dims = in[i]->nr_dims;
		out[i]->data_type = in[i]->data_type;
		out[i]->owned = 1;
		out[i]->size = in[i]->size;
		out[i]->dims = malloc(sizeof(*out[i]->dims) * in[i]->nr_dims);
		out[i]->data = malloc(in[i]->size * sizeof(double));
		if (!out[i]->dims || !out[i]->data)
			goto free_tflite;
		memcpy(out[i]->dims, in[i]->dims,
		       sizeof(*in[i]->dims) * in[i]->nr_dims);
		memcpy(out[i]->data, in[i]->data, in[i]->size);
	}

	if (status)
		*status = 0;

	return VACCEL_OK;

free_tflite:
	for (int i = 0; i < nr_outputs; ++i) {
		if (!out[i])
			continue;

		if (out[i]->dims)
			free(out[i]->dims);
		if (out[i]->data)
			free(out[i]->data);

		free(out[i]);
	}
	return VACCEL_ENOMEM;
}

static int noop_torch_model_load(struct vaccel_session *sess,
				 const struct vaccel_resource *model)
{
	if (!sess) {
		noop_error("Invalid session");
		return VACCEL_EINVAL;
	}

	if (!model) {
		noop_error("Invalid model path");
		return VACCEL_EINVAL;
	}

	noop_debug("Calling torch_model_load for session %" PRId64 "",
		   sess->id);

	int ret = VACCEL_OK;
	struct vaccel_blob *blob = model->blobs[0];
	switch (blob->type) {
	case VACCEL_BLOB_FILE:
		noop_debug("Loading model from file %s", blob->path);
		break;
	case VACCEL_BLOB_MAPPED:
		noop_debug("Mapped model, loading from file %s", blob->path);
		break;
	case VACCEL_BLOB_BUFFER:
		noop_debug("Loading model from buffer of length: %u",
			   blob->size);
		break;
	default:
		noop_error("The model blob has an invalid type.");
		ret = VACCEL_EINVAL;
		break;
	}

	return ret;
}

static int noop_torch_model_run(struct vaccel_session *sess,
				const struct vaccel_resource *model,
				const struct vaccel_torch_buffer *run_options,
				struct vaccel_torch_tensor **in_tensor,
				int nr_read,
				struct vaccel_torch_tensor **out_tensor,
				int nr_write)
{
	(void)run_options;

	if (!sess) {
		noop_error("Invalid session");
		return VACCEL_EINVAL;
	}

	if (!model) {
		noop_error("Invalid model path");
		return VACCEL_EINVAL;
	}

	noop_debug("Calling torch_model_run for session %" PRId64 "", sess->id);

	noop_debug("Number of inputs: %d", nr_read);
	for (int i = 0; i < nr_read; ++i) {
		noop_debug("\t#dims: %" PRId64 " -> {", in_tensor[i]->nr_dims);
		for (int j = 0; j < in_tensor[i]->nr_dims; ++j)
			noop_debug("\t\t%" PRId64, in_tensor[i]->dims[j]);
		noop_debug("\t}");

		noop_debug("\tData type: %d", in_tensor[i]->data_type);
		noop_debug("\tData -> %p, %zu B", in_tensor[i]->data,
			   in_tensor[i]->size);
	}

	noop_debug("Number of outputs: %d", nr_write);
	for (int i = 0; i < nr_write; ++i) {
		out_tensor[i] = NULL;
	}
	for (int i = 0; i < nr_write; ++i) {
		out_tensor[i] = malloc(sizeof(*out_tensor[i]));
		if (!out_tensor[i])
			goto free_torch;
		out_tensor[i]->nr_dims = in_tensor[i]->nr_dims;
		out_tensor[i]->data_type = in_tensor[i]->data_type;
		out_tensor[i]->owned = 1;
		out_tensor[i]->size = in_tensor[i]->size;
		out_tensor[i]->dims = malloc(sizeof(*out_tensor[i]->dims) *
					     in_tensor[i]->nr_dims);
		out_tensor[i]->data =
			malloc(in_tensor[i]->size * sizeof(double));
		if (!out_tensor[i]->dims || !out_tensor[i]->data)
			goto free_torch;
		memcpy(out_tensor[i]->dims, in_tensor[i]->dims,
		       sizeof(*in_tensor[i]->dims) * in_tensor[i]->nr_dims);
		memcpy(out_tensor[i]->data, in_tensor[i]->data,
		       in_tensor[i]->size);
	}

	return VACCEL_OK;

free_torch:
	for (int i = 0; i < nr_write; ++i) {
		if (!out_tensor[i])
			continue;

		if (out_tensor[i]->dims)
			free(out_tensor[i]->dims);
		if (out_tensor[i]->data)
			free(out_tensor[i]->data);

		free(out_tensor[i]);
	}
	return VACCEL_ENOMEM;
}

static int noop_torch_sgemm(struct vaccel_session *sess,
			    struct vaccel_torch_tensor **in_A,
			    struct vaccel_torch_tensor **in_B,
			    struct vaccel_torch_tensor **in_C, int M, int N,
			    int K, struct vaccel_torch_tensor **out)
{
	(void)in_A;
	(void)in_B;
	(void)in_C;
	(void)out;

	if (!sess) {
		noop_error("Invalid session");
		return VACCEL_EINVAL;
	}

	noop_debug("Calling torch_sgemm for session %" PRId64 "", sess->id);
	noop_debug("Dumping arguments for torch_sgemm:");
	noop_debug("m: %d n: %d k: %d", M, N, K);
	return VACCEL_OK;
}

static int noop_sgemm(struct vaccel_session *sess, long long int m,
		      long long int n, long long int k, float alpha, float *a,
		      long long int lda, float *b, long long int ldb,
		      float beta, float *c, long long int ldc)
{
	noop_debug("Calling sgemm for session %" PRId64 "", sess->id);

	noop_debug("Dumping arguments for sgemm:");
	noop_debug("m: %lld n: %lld k: %lld", m, n, k);
	noop_debug("alpha: %f", alpha);
	noop_debug("A: %p lda: %lld", a, lda);
	noop_debug("B: %p ldb: %lld", b, ldb);
	noop_debug("beta: %f", beta);
	noop_debug("C: %p ldc: %lld", c, ldc);

	return VACCEL_OK;
}

int noop_fpga_arraycopy(struct vaccel_session *sess, int a[], int out_a[],
			size_t len_a)
{
	noop_debug("Calling fpga_arraycopy for session %" PRId64 "", sess->id);

	noop_debug("Dumping arguments for fpga_arraycopy:");
	noop_debug("len_a: %zu ", len_a);

	/* Fill output with dummy values */
	for (size_t i = 0; i < len_a; i++) {
		out_a[i] = a[i];
	}

	return VACCEL_OK;
}

int noop_fpga_mmult(struct vaccel_session *sess, float a[], float b[],
		    float c[], size_t len_a)
{
	noop_debug("Calling fpga_mmult for session %" PRId64 "", sess->id);

	noop_debug("Dumping arguments for fpga_mmult:");
	noop_debug("len_a: %zu", len_a);

	/* Fill output with dummy values */
	const float dummy_val = 9.1F;
	for (size_t i = 0; i < len_a; i++) {
		c[i] = dummy_val;
	}

	return VACCEL_OK;
}

int noop_fpga_parallel(struct vaccel_session *sess, float a[], float b[],
		       float add_output[], float mult_output[], size_t len_a)
{
	noop_debug("Calling fpga_parallel for session %" PRId64 "", sess->id);

	noop_debug("Dumping arguments for fpga_parallel:");
	noop_debug("len_a: %zu", len_a);

	/* Fill output with dummy values */
	for (size_t i = 0; i < len_a; i++) {
		add_output[i] = a[i] + b[i];
		mult_output[i] = 1;
	}

	return VACCEL_OK;
}

int noop_fpga_vadd(struct vaccel_session *sess, float a[], float b[], float c[],
		   size_t len_a, size_t len_b)
{
	(void)len_b;

	noop_debug("Calling fpga_vadd for session %" PRId64 "", sess->id);

	noop_debug("Dumping arguments for fpga_vadd:");
	noop_debug("len_a: %zu len_b: %zu ", len_a, len_b);

	/* Fill output with dummy values */
	for (size_t i = 0; i < len_a; i++) {
		c[i] = a[i] + b[i];
	}

	return VACCEL_OK;
}

int noop_minmax(struct vaccel_session *sess, const double *indata, int ndata,
		int low_threshold, int high_threshold, double *outdata,
		double *min, double *max)
{
	const double tmp_max = 10000.0;
	const double tmp_min = -1.0;

	if (!sess)
		return VACCEL_EINVAL;

	noop_debug("Calling minmax for session %" PRId64 "", sess->id);

	noop_debug("Dumping arguments for minmax: ndata:%d", ndata);
	noop_debug("low: %d high: %d ", low_threshold, high_threshold);

	noop_debug("will return dummy min and max values");
	memcpy(outdata, indata, ndata * sizeof(double));
	*max = tmp_max;
	*min = tmp_min;

	return VACCEL_OK;
}

static int noop_opencv(struct vaccel_session *sess, struct vaccel_arg *read,
		       size_t nr_read, struct vaccel_arg *write,
		       size_t nr_write)
{
	noop_debug("Calling opencv for session %" PRId64 "", sess->id);

	noop_debug("Dumping arguments for opencv:");
	noop_debug("nr_read: %zu nr_write: %zu", nr_read, nr_write);
	noop_debug("[OpenCV] function: %u", *(uint8_t *)read[0].buf);
	for (size_t i = 1; i < nr_read; i++) {
		noop_debug("opencv read[%zu] size: %u", i, read[i].size);
	}
	for (size_t i = 0; i < nr_write; i++) {
		noop_debug("opencv write[%zu] size: %u", i, write[i].size);
		size_t *header;
		header = write[i].buf;
		header[0] = write[i].size / sizeof(float);
	}

	return VACCEL_OK;
}

struct vaccel_op ops[] = {
	VACCEL_OP_INIT(ops[0], VACCEL_OP_NOOP, noop_noop),
	VACCEL_OP_INIT(ops[1], VACCEL_OP_EXEC, noop_exec),
	VACCEL_OP_INIT(ops[2], VACCEL_OP_EXEC_WITH_RESOURCE,
		       noop_exec_with_resource),
	VACCEL_OP_INIT(ops[3], VACCEL_OP_IMAGE_CLASSIFY, noop_image_classify),
	VACCEL_OP_INIT(ops[4], VACCEL_OP_IMAGE_DETECT, noop_image_detect),
	VACCEL_OP_INIT(ops[5], VACCEL_OP_IMAGE_SEGMENT, noop_image_segment),
	VACCEL_OP_INIT(ops[6], VACCEL_OP_IMAGE_POSE, noop_image_pose),
	VACCEL_OP_INIT(ops[7], VACCEL_OP_IMAGE_DEPTH, noop_image_depth),
	VACCEL_OP_INIT(ops[8], VACCEL_OP_TF_MODEL_LOAD, noop_tf_model_load),
	VACCEL_OP_INIT(ops[9], VACCEL_OP_TF_MODEL_UNLOAD, noop_tf_model_unload),
	VACCEL_OP_INIT(ops[10], VACCEL_OP_TF_MODEL_RUN, noop_tf_model_run),
	VACCEL_OP_INIT(ops[11], VACCEL_OP_TFLITE_MODEL_LOAD,
		       noop_tflite_model_load),
	VACCEL_OP_INIT(ops[12], VACCEL_OP_TFLITE_MODEL_UNLOAD,
		       noop_tflite_model_unload),
	VACCEL_OP_INIT(ops[13], VACCEL_OP_TFLITE_MODEL_RUN,
		       noop_tflite_model_run),
	VACCEL_OP_INIT(ops[14], VACCEL_OP_TORCH_MODEL_LOAD,
		       noop_torch_model_load),
	VACCEL_OP_INIT(ops[15], VACCEL_OP_TORCH_MODEL_RUN,
		       noop_torch_model_run),
	VACCEL_OP_INIT(ops[16], VACCEL_OP_TORCH_SGEMM, noop_torch_sgemm),
	VACCEL_OP_INIT(ops[17], VACCEL_OP_BLAS_SGEMM, noop_sgemm),
	VACCEL_OP_INIT(ops[18], VACCEL_OP_FPGA_ARRAYCOPY, noop_fpga_arraycopy),
	VACCEL_OP_INIT(ops[19], VACCEL_OP_FPGA_VECTORADD, noop_fpga_vadd),
	VACCEL_OP_INIT(ops[20], VACCEL_OP_FPGA_PARALLEL, noop_fpga_parallel),
	VACCEL_OP_INIT(ops[21], VACCEL_OP_FPGA_MMULT, noop_fpga_mmult),
	VACCEL_OP_INIT(ops[22], VACCEL_OP_MINMAX, noop_minmax),
	VACCEL_OP_INIT(ops[23], VACCEL_OP_OPENCV, noop_opencv),
};

static int init(void)
{
	return vaccel_plugin_register_ops(ops, sizeof(ops) / sizeof(ops[0]));
}

static int fini(void)
{
	return VACCEL_OK;
}

VACCEL_PLUGIN(.name = "noop", .version = VACCEL_VERSION,
	      .vaccel_version = VACCEL_VERSION, .type = VACCEL_PLUGIN_DEBUG,
	      .init = init, .fini = fini)
