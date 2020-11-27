#include "vaccel_ops.h"
#include "backend.h"
#include "common.h"

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

	//Get implementation
	noop_t *backend_op = get_backend_op(VACCEL_NO_OP);
	if (!backend_op)
		return VACCEL_ENOTSUP;

	return backend_op(sess);
}

int vaccel_image_classification(struct vaccel_session *sess, void *img,
		unsigned char *out_text, unsigned char *out_imgname,
		size_t len_img, size_t len_out_text, size_t len_out_imgname)
{
	if (!sess)
		return VACCEL_EINVAL;

	//Get implementation
	image_classification_t *backend_op = get_backend_op(VACCEL_IMG_CLASS);
	if (!backend_op)
		return VACCEL_ENOTSUP;

	return backend_op(sess, img, out_text, out_imgname, len_img,
			len_out_text, len_out_imgname);
}
