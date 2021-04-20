#ifndef __VACCEL_IMAGE_OPS_H__
#define __VACCEL_IMAGE_OPS_H__

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

struct vaccel_session;

int vaccel_image_classification(struct vaccel_session *sess, const void *img,
		unsigned char *out_text, unsigned char *out_imgname,
		size_t len_img, size_t len_out_text, size_t len_out_imgname);

int vaccel_image_detection(struct vaccel_session *sess, const void *img,
		const unsigned char *out_imgname, size_t len_img,
		size_t len_out_imgname);

int vaccel_image_segmentation(struct vaccel_session *sess, const void *img,
		const unsigned char *out_imgname, size_t len_img,
		size_t len_out_imgname);

#ifdef __cplusplus
}
#endif

#endif /* __VACCEL_IMAGE_OPS_H__ */
