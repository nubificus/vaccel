#ifndef __IMAGE_OPS_H__
#define __IMAGE_OPS_H__

#include "include/ops/image.h"

struct vaccel_session;
struct vaccel_arg;

int vaccel_image_classification_unpack(struct vaccel_session *sess,
		struct vaccel_arg *read, int nr_read,
		struct vaccel_arg *write, int nr_write);

int vaccel_image_detection_unpack(struct vaccel_session *sess,
		struct vaccel_arg *read, int nr_read,
		struct vaccel_arg *write, int nr_write);

int vaccel_image_segmentation_unpack(struct vaccel_session *sess,
		struct vaccel_arg *read, int nr_read,
		struct vaccel_arg *write, int nr_write);

#endif /* __IMAGE_OPS_H__ */
