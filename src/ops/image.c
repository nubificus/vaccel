#include "image.h"
#include "error.h"
#include "plugin.h"
#include "log.h"
#include "vaccel_ops.h"
#include "genop.h"

#include "session.h"

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

int vaccel_image_classification_unpack(struct vaccel_session *sess,
		struct vaccel_arg *read, int nr_read,
		struct vaccel_arg *write, int nr_write)
{
	if (nr_read != 1) {
		vaccel_error("Wrong number of read arguments in image classification: %d",
				nr_read);
		return VACCEL_EINVAL;
	}

	if (nr_write != 2) {
		vaccel_error("Wrong number of write arguments in image classification: %d",
				nr_write);
		return VACCEL_EINVAL;
	}

	void *img = (void *)read[0].buf;
	size_t len_img = (size_t)read[0].size;

	unsigned char *out_text = (unsigned char *)write[0].buf;
	size_t len_out_text = (size_t)write[0].size;
	unsigned char *out_imgname = (unsigned char *)write[1].buf;
	size_t len_out_imgname = (size_t)write[1].size;

	return vaccel_image_classification(sess, img, out_text, out_imgname,
			len_img, len_out_text, len_out_imgname);
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

int vaccel_image_detection_unpack(struct vaccel_session *sess,
		struct vaccel_arg *read, int nr_read,
		struct vaccel_arg *write, int nr_write)
{
	if (nr_read != 1) {
		vaccel_error("Wrong number of read arguments in image classification: %d",
				nr_read);
		return VACCEL_EINVAL;
	}

	if (nr_write != 1) {
		vaccel_error("Wrong number of write arguments in image classification: %d",
				nr_write);
		return VACCEL_EINVAL;
	}

	void *img = (void *)read[0].buf;
	size_t len_img = (size_t)read[0].size;

	unsigned char *out_imgname = (unsigned char *)write[0].buf;
	size_t len_out_imgname = (size_t)write[0].size;

	return vaccel_image_detection(sess, img, out_imgname, len_img,
			len_out_imgname);
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

int vaccel_image_segmentation_unpack(struct vaccel_session *sess,
		struct vaccel_arg *read, int nr_read,
		struct vaccel_arg *write, int nr_write)
{
	if (nr_read != 1) {
		vaccel_error("Wrong number of read arguments in image classification: %d",
				nr_read);
		return VACCEL_EINVAL;
	}

	if (nr_write != 1) {
		vaccel_error("Wrong number of write arguments in image classification: %d",
				nr_write);
		return VACCEL_EINVAL;
	}

	void *img = (void *)read[0].buf;
	size_t len_img = (size_t)read[0].size;

	unsigned char *out_imgname = (unsigned char *)write[0].buf;
	size_t len_out_imgname = (size_t)write[0].size;

	return vaccel_image_segmentation(sess, img, out_imgname, len_img,
			len_out_imgname);
}
