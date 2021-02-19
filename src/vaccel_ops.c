#include "vaccel_ops.h"
#include "plugin.h"
#include "common.h"
#include "log.h"
#include "session.h"

/* export supported function as types for the rest of the runtime */
typedef typeof(vaccel_noop) noop_t;
typedef typeof(vaccel_sgemm) sgemm_t;
typedef typeof(vaccel_image_classification) image_classification_t;
typedef typeof(vaccel_image_detection) image_detection_t;
typedef typeof(vaccel_image_segmentation) image_segmentation_t;

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
