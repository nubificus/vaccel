#ifndef __VACCEL_ML_OPS_H__
#define __VACCEL_ML_OPS_H__

#include <stddef.h>

struct vaccel_session;
struct vaccel_ml_caffe_model;

int vaccel_image_classification(struct vaccel_session *sess,
		struct vaccel_ml_caffe_model *model, const void *img,
		size_t img_len, unsigned char *out_text, size_t out_text_len);

int vaccel_image_detection(struct vaccel_session *sess, void *img,
		unsigned char *out_imgname, size_t len_img,
		size_t len_out_imgname);

int vaccel_image_segmentation(struct vaccel_session *sess, void *img,
		unsigned char *out_imgname, size_t len_img,
		size_t len_out_imgname);


#endif /* __VACCEL_ML_OPS_H__ */
