#include "plugin.h"
#include "vaccel.h"

extern int sess_init(struct vaccel_session *, uint32_t);
extern int sess_free(struct vaccel_session *);
extern int image_classify(
	struct vaccel_session *,
	const unsigned char *img,
	size_t img_len,
	unsigned char *tags,
	size_t tags_len
);
extern int register_caffe_model(
	struct vaccel_session *,
	struct vaccel_ml_caffe_model *
);
extern int unregister_resource(
	struct vaccel_session *,
	struct vaccel_resource *
);

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

int register_resource(
	struct vaccel_session *sess,
	enum vaccel_resource_type type,
	struct vaccel_resource *res
) {
	if (!sess)
		return VACCEL_EINVAL;

	if (!res)
		return VACCEL_EINVAL;

	switch (type) {
		case VACCEL_ML_CAFFE_MODEL:
			return register_caffe_model(sess,
					(struct vaccel_ml_caffe_model *)res);
		default:
			return VACCEL_EINVAL;
	}
}

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
	.register_resource = register_resource,
	.unregister_resource = unregister_resource,
)
