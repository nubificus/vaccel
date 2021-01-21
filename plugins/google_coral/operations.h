#ifndef __VACCEL_JETSON_OPERATIONS_H__
#define __VACCEL_JETSON_OPERATIONS_H__

#include <stddef.h>

int coral_image_classification(struct vaccel_session *sess, void *img,
		char *out_text, char *out_imgname,
		size_t len_img, size_t len_out_text, size_t len_out_imgname);
int coral_image_detect(struct vaccel_session *sess, void *img,
		char *out_text, char *out_imgname,
		size_t len_img, size_t len_out_text, size_t len_out_imgname);
int coral_image_segment(struct vaccel_session *sess, void *img,
		char *out_text, char *out_imgname,
		size_t len_img, size_t len_out_text, size_t len_out_imgname);

#endif /* __VACCEL_JETSON_OPERATIONS_H__ */
