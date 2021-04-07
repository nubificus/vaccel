#ifndef __VACCEL_IMAGE_OPS_H__
#define __VACCEL_IMAGE_OPS_H__

#include <stddef.h>

struct vaccel_session;
struct vaccel_arg;

int vaccel_image_classification(struct vaccel_session *sess, const void *img,
		unsigned char *out_text, unsigned char *out_imgname,
		size_t len_img, size_t len_out_text, size_t len_out_imgname);
int vaccel_image_classification_unpack(struct vaccel_session *sess,
		struct vaccel_arg *read, int nr_read,
		struct vaccel_arg *write, int nr_write);

int vaccel_image_detection(struct vaccel_session *sess, const void *img,
		const unsigned char *out_imgname, size_t len_img,
		size_t len_out_imgname);
int vaccel_image_detection_unpack(struct vaccel_session *sess,
		struct vaccel_arg *read, int nr_read,
		struct vaccel_arg *write, int nr_write);

int vaccel_image_segmentation(struct vaccel_session *sess, const void *img,
		const unsigned char *out_imgname, size_t len_img,
		size_t len_out_imgname);
int vaccel_image_segmentation_unpack(struct vaccel_session *sess,
		struct vaccel_arg *read, int nr_read,
		struct vaccel_arg *write, int nr_write);


#endif /* __VACCEL_IMAGE_OPS_H__ */
