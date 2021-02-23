#include "ml_ops.h"
#include "plugin.h"
#include "log.h"

typedef typeof(vaccel_image_classification) image_classification_t;
typedef typeof(vaccel_image_detection) image_detection_t;
typedef typeof(vaccel_image_segmentation) image_segmentation_t;

int vaccel_image_classification(struct vaccel_session *sess,
		struct vaccel_ml_caffe_model *model, const void *img,
		size_t img_len, unsigned char *tags, size_t tags_len)
{
	if (!sess)
		return VACCEL_EINVAL;

	vaccel_debug("session:%u Looking for plugin implementing image classification",
			sess->session_id);

	//Get implementation
	image_classification_t *plugin_op = get_plugin_op(VACCEL_IMG_CLASS);
	if (!plugin_op)
		return VACCEL_ENOTSUP;

	return plugin_op(sess, model, img, img_len, tags, tags_len);
}
