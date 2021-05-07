#include <stdio.h>
#include <string.h>

#include <plugin.h>
#include <error.h>
#include <ops/vaccel_ops.h>

static int noop_noop(struct vaccel_session *sess)
{
	fprintf(stdout, "[noop] Calling no-op for session %u\n",
		sess->session_id);

	return VACCEL_OK;
}

static int noop_sgemm(struct vaccel_session *sess, uint32_t k, uint32_t m,
		      uint32_t n, size_t len_a, size_t len_b, size_t len_c,
		      float *a, float *b, float *c)
{
	fprintf(stdout, "[noop] Calling sgemm for session %u\n",
		sess->session_id);

	fprintf(stdout, "[noop] Dumping arguments for sgemm:\n");
	fprintf(stdout, "[noop] k: %u m: %u n: %u\n", k, m, n);
	fprintf(stdout, "[noop] len_a: %lu len_b: %lu len_c: %lu\n", len_a, len_b, len_c);
	
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

struct vaccel_op ops[] = {
	VACCEL_OP_INIT(ops[0], VACCEL_NO_OP, noop_noop),
	VACCEL_OP_INIT(ops[1], VACCEL_BLAS_SGEMM, noop_sgemm),
	VACCEL_OP_INIT(ops[2], VACCEL_IMG_CLASS, noop_img_class),
	VACCEL_OP_INIT(ops[3], VACCEL_IMG_DETEC, noop_img_detect),
	VACCEL_OP_INIT(ops[4], VACCEL_IMG_SEGME, noop_img_segme),
	VACCEL_OP_INIT(ops[5], VACCEL_EXEC, noop_exec),
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
