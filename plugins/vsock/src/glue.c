#include "plugin.h"
#include "vaccel.h"
#include "ops/vaccel_ops.h"

extern int sess_init(struct vaccel_session *, uint32_t);
extern int sess_free(struct vaccel_session *);
extern int image_classify(struct vaccel_session *, const unsigned char *img,
		size_t img_len, unsigned char *tags, size_t tags_len);

int vsock_img_classify(struct vaccel_session *session, void *img,
		char *out_text, char *out_imgname, size_t len_img,
		size_t len_out_text, size_t len_out_imgname)
{
	(void)out_imgname;
	(void)len_out_imgname;
	return image_classify(session, img, len_img, (unsigned char *)out_text,
			len_out_text);
}

struct vaccel_op ops[] = {
	VACCEL_OP_INIT(ops[0], VACCEL_IMG_CLASS, vsock_img_classify),
};

int vsock_init(void)
{
	int ret = register_plugin_functions(ops, sizeof(ops) / sizeof(ops[0]));
	if (!ret)
		return ret;

	return VACCEL_OK;
}

int vsock_fini(void)
{
	return VACCEL_OK;
}

VACCEL_MODULE(
	.name = "vsock",
	.version = "0.1",
	.init = vsock_init,
	.fini = vsock_init,
	.sess_init = sess_init,
	.sess_free = sess_free,
)
