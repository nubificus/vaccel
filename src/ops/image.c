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

#include "image.h"
#include "error.h"
#include "plugin.h"
#include "log.h"
#include "vaccel_ops.h"
#include "genop.h"
#include "session.h"
#include "vaccel_prof.h"

struct vaccel_prof_region image_op_stats =
	VACCEL_PROF_REGION_INIT("vaccel_image_op");

int vaccel_image_op(enum vaccel_op_type op_type, struct vaccel_session *sess,
		const void *img, unsigned char *out_text,
		unsigned char *out_imgname, size_t len_img,
		size_t len_out_text, size_t len_out_imgname)
{
	int ret;

	if (!sess)
		return VACCEL_EINVAL;

	vaccel_debug("session:%u Looking for plugin implementing %s",
			sess->session_id, vaccel_op_type_str(op_type));

	vaccel_prof_region_start(&image_op_stats);

	//Get implementation
	int (*plugin_op)() = get_plugin_op(op_type, sess->hint);
	if (!plugin_op) {
		ret = VACCEL_ENOTSUP;
		goto out;
	}

	if (out_text != NULL && len_out_text > 0) {
		ret = plugin_op(sess, img, out_text, out_imgname, len_img,
				len_out_text, len_out_imgname);
	} else {
		ret = plugin_op(sess, img, out_imgname, len_img,
				len_out_imgname);
	}

out:
	vaccel_prof_region_stop(&image_op_stats);
	return ret;
}

#define vaccel_image_op_no_text(op_type, sess, img, out_imgname, len_img, \
		len_out_imgname) \
		vaccel_image_op(op_type, sess, img, NULL, out_imgname, \
				len_img, 0, len_out_imgname)

int vaccel_image_op_unpack(enum vaccel_op_type op_type,
		struct vaccel_session *sess, struct vaccel_arg *read,
		int nr_read, int nr_read_req, struct vaccel_arg *write,
		int nr_write, int nr_write_req)
{
	if (nr_read != nr_read_req) {
		vaccel_error("Wrong number of read arguments in %s: %d (expected %d)",
				vaccel_op_type_str(op_type), nr_read,
				nr_read_req);
		return VACCEL_EINVAL;
	}

	if (nr_write != nr_write_req) {
		vaccel_error("Wrong number of write arguments in %s: %d (expected %d)",
				vaccel_op_type_str(op_type), nr_write,
				nr_write_req);
		return VACCEL_EINVAL;
	}

	void *img = (void *)read[0].buf;
	size_t len_img = (size_t)read[0].size;

	if (nr_write_req == 2) {
		unsigned char *out_text = (unsigned char *)write[0].buf;
		size_t len_out_text = (size_t)write[0].size;
		unsigned char *out_imgname = (unsigned char *)write[1].buf;
		size_t len_out_imgname = (size_t)write[1].size;

		return vaccel_image_op(op_type, sess, img, out_text,
				out_imgname, len_img, len_out_text,
				len_out_imgname);
	} else {
		unsigned char *out_imgname = (unsigned char *)write[0].buf;
		size_t len_out_imgname = (size_t)write[0].size;

		return vaccel_image_op_no_text(op_type, sess, img, out_imgname,
				len_img, len_out_imgname);
	}
}

int vaccel_image_classification(struct vaccel_session *sess, const void *img,
		unsigned char *out_text, unsigned char *out_imgname,
		size_t len_img, size_t len_out_text, size_t len_out_imgname)
{
	return vaccel_image_op(VACCEL_IMG_CLASS, sess, img, out_text,
			out_imgname, len_img, len_out_text, len_out_imgname);
}

int vaccel_image_classification_unpack(struct vaccel_session *sess,
		struct vaccel_arg *read, int nr_read,
		struct vaccel_arg *write, int nr_write)
{
	return vaccel_image_op_unpack(VACCEL_IMG_CLASS, sess, read, nr_read, 1,
			write, nr_write, 2);
}

int vaccel_image_detection(struct vaccel_session *sess, const void *img,
		unsigned char *out_imgname, size_t len_img,
		size_t len_out_imgname)
{
	return vaccel_image_op_no_text(VACCEL_IMG_DETEC, sess, img, out_imgname,
			len_img, len_out_imgname);
}

int vaccel_image_detection_unpack(struct vaccel_session *sess,
		struct vaccel_arg *read, int nr_read,
		struct vaccel_arg *write, int nr_write)
{
	return vaccel_image_op_unpack(VACCEL_IMG_DETEC, sess, read, nr_read, 1,
			write, nr_write, 1);
}

int vaccel_image_segmentation(struct vaccel_session *sess, const void *img,
		unsigned char *out_imgname, size_t len_img,
		size_t len_out_imgname)
{
	return vaccel_image_op_no_text(VACCEL_IMG_SEGME, sess, img, out_imgname,
			len_img, len_out_imgname);
}

int vaccel_image_segmentation_unpack(struct vaccel_session *sess,
		struct vaccel_arg *read, int nr_read,
		struct vaccel_arg *write, int nr_write)
{
	return vaccel_image_op_unpack(VACCEL_IMG_SEGME, sess, read, nr_read, 1,
			write, nr_write, 1);
}

int vaccel_image_pose(struct vaccel_session *sess, const void *img,
		unsigned char *out_imgname, size_t len_img,
		size_t len_out_imgname)
{
	return vaccel_image_op_no_text(VACCEL_IMG_POSE, sess, img, out_imgname,
			len_img, len_out_imgname);
}

int vaccel_image_pose_unpack(struct vaccel_session *sess,
		struct vaccel_arg *read, int nr_read,
		struct vaccel_arg *write, int nr_write)
{
	return vaccel_image_op_unpack(VACCEL_IMG_POSE, sess, read, nr_read, 1,
			write, nr_write, 1);
}

int vaccel_image_depth(struct vaccel_session *sess, const void *img,
		unsigned char *out_imgname, size_t len_img,
		size_t len_out_imgname)
{
	return vaccel_image_op_no_text(VACCEL_IMG_DEPTH, sess, img, out_imgname,
			len_img, len_out_imgname);
}

int vaccel_image_depth_unpack(struct vaccel_session *sess,
		struct vaccel_arg *read, int nr_read,
		struct vaccel_arg *write, int nr_write)
{
	return vaccel_image_op_unpack(VACCEL_IMG_DEPTH, sess, read, nr_read, 1,
			write, nr_write, 1);
}

__attribute__((constructor))
static void vaccel_tf_ops_init(void)
{
}

__attribute__((destructor))
static void vaccel_tf_ops_fini(void)
{
	vaccel_prof_region_print(&image_op_stats);
}
