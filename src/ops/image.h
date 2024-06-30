// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "include/ops/image.h"
#include "include/ops/vaccel_ops.h"

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

int vaccel_image_pose_unpack(struct vaccel_session *sess,
			     struct vaccel_arg *read, int nr_read,
			     struct vaccel_arg *write, int nr_write);

int vaccel_image_depth_unpack(struct vaccel_session *sess,
			      struct vaccel_arg *read, int nr_read,
			      struct vaccel_arg *write, int nr_write);
