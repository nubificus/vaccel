/*
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

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
		unsigned char *out_imgname, size_t len_img,
		size_t len_out_imgname);

int vaccel_image_segmentation(struct vaccel_session *sess, const void *img,
		unsigned char *out_imgname, size_t len_img,
		size_t len_out_imgname);

int vaccel_image_pose(struct vaccel_session *sess, const void *img,
		unsigned char *out_imgname, size_t len_img,
		size_t len_out_imgname);

int vaccel_image_depth(struct vaccel_session *sess, const void *img,
		unsigned char *out_imgname, size_t len_img,
		size_t len_out_imgname);

#ifdef __cplusplus
}
#endif

#endif /* __VACCEL_IMAGE_OPS_H__ */
