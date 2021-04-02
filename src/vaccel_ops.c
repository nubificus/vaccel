#include "vaccel_ops.h"
#include "plugin.h"
#include "common.h"
#include "log.h"
#include "vaccel.h"

/* export supported function as types for the rest of the runtime */
typedef typeof(vaccel_noop) noop_t;
typedef typeof(vaccel_sgemm) sgemm_t;
typedef typeof(vaccel_image_classification) image_classification_t;
typedef typeof(vaccel_image_detection) image_detection_t;
typedef typeof(vaccel_image_segmentation) image_segmentation_t;
typedef typeof(vaccel_genop) genop_t;

int vaccel_noop(struct vaccel_session *sess)
{
	if (!sess)
		return VACCEL_EINVAL;

	vaccel_debug("session:%u Looking for plugin implementing noop",
			sess->session_id);

	//Get implementation
	noop_t *plugin_op = get_plugin_op(VACCEL_NO_OP);
	if (!plugin_op)
		return VACCEL_ENOTSUP;

	return plugin_op(sess);
}

int vaccel_image_classification(struct vaccel_session *sess, const void *img,
		unsigned char *out_text, unsigned char *out_imgname,
		size_t len_img, size_t len_out_text, size_t len_out_imgname)
{
	if (!sess)
		return VACCEL_EINVAL;

	vaccel_debug("session:%u Looking for plugin implementing image classification",
			sess->session_id);

	//Get implementation
	image_classification_t *plugin_op = get_plugin_op(VACCEL_IMG_CLASS);
	if (!plugin_op)
		return VACCEL_ENOTSUP;

	return plugin_op(sess, img, out_text, out_imgname, len_img,
			len_out_text, len_out_imgname);
}

int vaccel_genop(struct vaccel_session *sess, void *out_args, void *in_args, size_t out_nargs, size_t in_nargs)
{
	if (!sess)
		return VACCEL_EINVAL;

	vaccel_debug("session:%u Looking for plugin implementing generic op",
			sess->session_id);

	//Get implementation
	genop_t *plugin_op = get_plugin_op(VACCEL_GEN_OP);
	if (!plugin_op)
		return VACCEL_ENOTSUP;

	return plugin_op(sess, out_args, in_args, out_nargs, in_nargs);
}


