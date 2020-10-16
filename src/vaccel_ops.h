#ifndef __VACCEL_OPS_H__
#define __VACCEL_OPS_H__

#include <stddef.h>

#define VACCEL_NO_OP        0
#define VACCEL_BLAS_SGEMM   1
#define VACCEL_IMG_CLASS    2
#define VACCEL_IMG_DETEC    3
#define VACCEL_IMG_SEGME    4
#define VACCEL_FUNCTIONS_NR 5

struct vaccel_session;

/* vaccel supported operations */
int vaccel_image_classification(struct vaccel_session *sess, void *img,
		unsigned char *out_text, unsigned char *out_imgname,
		size_t len_img, size_t len_out_text, size_t len_out_imgname);

int vaccel_image_detection(struct vaccel_session *sess, void *img,
		unsigned char *out_imgname, size_t len_img,
		size_t len_out_imgname);

int vaccel_image_segmentation(struct vaccel_session *sess, void *img,
		unsigned char *out_imgname, size_t len_img,
		size_t len_out_imgname);

/* export supported function as types for the rest of the runtime */
typedef typeof(vaccel_image_classification) image_classification_t;
typedef typeof(vaccel_image_detection) image_detection_t;
typedef typeof(vaccel_image_segmentation) image_segmentation_t;

#endif /* __VACCEL_OPS_H__ */
