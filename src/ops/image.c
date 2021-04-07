#include "image.h"
#include "common.h"
#include "plugin.h"
#include "log.h"
#include "vaccel_ops.h"

int vaccel_image_classification(struct vaccel_session *sess, const void *img,
		unsigned char *out_text, unsigned char *out_imgname,
		size_t len_img, size_t len_out_text, size_t len_out_imgname)
{
	if (!sess)
		return VACCEL_EINVAL;

	vaccel_debug("session:%u Looking for plugin implementing image classification",
			sess->session_id);

	//Get implementation
	int (*plugin_op)() = get_plugin_op(VACCEL_IMG_CLASS);
	if (!plugin_op)
		return VACCEL_ENOTSUP;

	return plugin_op(sess, img, out_text, out_imgname, len_img,
			len_out_text, len_out_imgname);
}

int vaccel_image_detection(struct vaccel_session *sess, const void *img,
		const unsigned char *out_imgname, size_t len_img,
		size_t len_out_imgname)
{
	if (!sess)
		return VACCEL_EINVAL;

	vaccel_debug("session:%u Looking for plugin implementing image detection",
			sess->session_id);

	//Get implementation
	int (*plugin_op)() = get_plugin_op(VACCEL_IMG_DETEC);
	if (!plugin_op)
		return VACCEL_ENOTSUP;

	return plugin_op(sess, img, out_imgname, len_img, len_out_imgname);
}

int vaccel_image_segmentation(struct vaccel_session *sess, const void *img,
		const unsigned char *out_imgname, size_t len_img,
		size_t len_out_imgname)
{
	if (!sess)
		return VACCEL_EINVAL;

	vaccel_debug("session:%u Looking for plugin implementing image segmentation",
			sess->session_id);

	//Get implementation
	int (*plugin_op)() = get_plugin_op(VACCEL_IMG_SEGME);
	if (!plugin_op)
		return VACCEL_ENOTSUP;

	return plugin_op(sess, img, out_imgname, len_img, len_out_imgname);
}
