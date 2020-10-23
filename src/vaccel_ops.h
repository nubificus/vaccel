#ifndef __VACCEL_OPS_H__
#define __VACCEL_OPS_H__

#include <stddef.h>
#include <stdint.h>

#define VACCEL_NO_OP        0
#define VACCEL_BLAS_SGEMM   1
#define VACCEL_IMG_CLASS    2
#define VACCEL_IMG_DETEC    3
#define VACCEL_IMG_SEGME    4
#define VACCEL_FUNCTIONS_NR 5

struct vaccel_session;

/* vaccel supported operations */
int vaccel_noop(struct vaccel_session *sess);
int vaccel_sgemm(struct vaccel_session *sess,
		uint32_t k, uint32_t m, uint32_t n,
		size_t len_a, size_t len_b, size_t len_c,
		float *a, float *b, float *c);
int vaccel_image_classification(struct vaccel_session *sess, void *img,
		char *out_text, char *out_imgname, size_t len_img,
		size_t len_out_text, size_t len_out_imgname);

int vaccel_image_detection(struct vaccel_session *sess, void *img,
		char *out_imgname, size_t len_img, size_t len_out_imgname);

int vaccel_image_segmentation(struct vaccel_session *sess, void *img,
		char *out_imgname, size_t len_img, size_t len_out_imgname);

#endif /* __VACCEL_OPS_H__ */
