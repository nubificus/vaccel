// SPDX-License-Identifier: Apache-2.0

#include "image.h"
#include "arg.h"
#include "error.h"
#include "log.h"
#include "op.h"
#include "plugin.h"
#include "prof.h"
#include "session.h"
#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>

struct vaccel_prof_region image_op_stats =
	VACCEL_PROF_REGION_INIT("vaccel_image_op");

typedef int (*image_op_no_text_fn_t)(struct vaccel_session *sess,
				     const void *img,
				     unsigned char *out_imgname, size_t len_img,
				     size_t len_out_imgname);
typedef int (*image_op_fn_t)(struct vaccel_session *sess, const void *img,
			     unsigned char *out_text,
			     unsigned char *out_imgname, size_t len_img,
			     size_t len_out_text, size_t len_out_imgname);

int vaccel_image_op(vaccel_op_type_t op_type, struct vaccel_session *sess,
		    const void *img, unsigned char *out_text,
		    unsigned char *out_imgname, size_t len_img,
		    size_t len_out_text, size_t len_out_imgname)
{
	int ret;

	if (!sess)
		return VACCEL_EINVAL;

	vaccel_debug("session:%" PRId64 " Looking for plugin implementing %s",
		     sess->id, vaccel_op_type_to_str(op_type));

	vaccel_prof_region_start(&image_op_stats);

	int (*plugin_image_op)() = plugin_get_op_func(op_type, sess->hint);
	if (!plugin_image_op) {
		ret = VACCEL_ENOTSUP;
		goto out;
	}

	if (out_text != NULL && len_out_text > 0) {
		ret = ((image_op_fn_t)plugin_image_op)(sess, img, out_text,
						       out_imgname, len_img,
						       len_out_text,
						       len_out_imgname);
	} else {
		ret = ((image_op_no_text_fn_t)plugin_image_op)(
			sess, img, out_imgname, len_img, len_out_imgname);
	}

out:
	vaccel_prof_region_stop(&image_op_stats);

	return ret;
}

#define vaccel_image_op_no_text(op_type, sess, img, out_imgname, len_img,  \
				len_out_imgname)                           \
	vaccel_image_op(op_type, sess, img, NULL, out_imgname, len_img, 0, \
			len_out_imgname)

int vaccel_image_op_unpack(vaccel_op_type_t op_type,
			   struct vaccel_session *sess, struct vaccel_arg *read,
			   int nr_read, int nr_read_req,
			   struct vaccel_arg *write, int nr_write,
			   int nr_write_req)
{
	if (nr_read != nr_read_req) {
		vaccel_error(
			"Wrong number of read arguments in %s: %d (expected %d)",
			vaccel_op_type_to_str(op_type), nr_read, nr_read_req);
		return VACCEL_EINVAL;
	}

	if (nr_write != nr_write_req) {
		vaccel_error(
			"Wrong number of write arguments in %s: %d (expected %d)",
			vaccel_op_type_to_str(op_type), nr_write, nr_write_req);
		return VACCEL_EINVAL;
	}

	void *img = read[0].buf;
	size_t len_img = (size_t)read[0].size;

	if (nr_write_req == 2) {
		unsigned char *out_text = (unsigned char *)write[0].buf;
		size_t len_out_text = (size_t)write[0].size;
		unsigned char *out_imgname = (unsigned char *)write[1].buf;
		size_t len_out_imgname = (size_t)write[1].size;

		return vaccel_image_op(op_type, sess, img, out_text,
				       out_imgname, len_img, len_out_text,
				       len_out_imgname);
	}
	unsigned char *out_imgname = (unsigned char *)write[0].buf;
	size_t len_out_imgname = (size_t)write[0].size;

	return vaccel_image_op_no_text(op_type, sess, img, out_imgname, len_img,
				       len_out_imgname);
}

int vaccel_image_classification(struct vaccel_session *sess, const void *img,
				unsigned char *out_text,
				unsigned char *out_imgname, size_t len_img,
				size_t len_out_text, size_t len_out_imgname)
{
	return vaccel_image_op(VACCEL_OP_IMAGE_CLASSIFY, sess, img, out_text,
			       out_imgname, len_img, len_out_text,
			       len_out_imgname);
}

int vaccel_image_classification_unpack(struct vaccel_session *sess,
				       struct vaccel_arg *read, int nr_read,
				       struct vaccel_arg *write, int nr_write)
{
	return vaccel_image_op_unpack(VACCEL_OP_IMAGE_CLASSIFY, sess, read,
				      nr_read, 1, write, nr_write, 2);
}

int vaccel_image_detection(struct vaccel_session *sess, const void *img,
			   unsigned char *out_imgname, size_t len_img,
			   size_t len_out_imgname)
{
	return vaccel_image_op_no_text(VACCEL_OP_IMAGE_DETECT, sess, img,
				       out_imgname, len_img, len_out_imgname);
}

int vaccel_image_detection_unpack(struct vaccel_session *sess,
				  struct vaccel_arg *read, int nr_read,
				  struct vaccel_arg *write, int nr_write)
{
	return vaccel_image_op_unpack(VACCEL_OP_IMAGE_DETECT, sess, read,
				      nr_read, 1, write, nr_write, 1);
}

int vaccel_image_segmentation(struct vaccel_session *sess, const void *img,
			      unsigned char *out_imgname, size_t len_img,
			      size_t len_out_imgname)
{
	return vaccel_image_op_no_text(VACCEL_OP_IMAGE_SEGMENT, sess, img,
				       out_imgname, len_img, len_out_imgname);
}

int vaccel_image_segmentation_unpack(struct vaccel_session *sess,
				     struct vaccel_arg *read, int nr_read,
				     struct vaccel_arg *write, int nr_write)
{
	return vaccel_image_op_unpack(VACCEL_OP_IMAGE_SEGMENT, sess, read,
				      nr_read, 1, write, nr_write, 1);
}

int vaccel_image_pose(struct vaccel_session *sess, const void *img,
		      unsigned char *out_imgname, size_t len_img,
		      size_t len_out_imgname)
{
	return vaccel_image_op_no_text(VACCEL_OP_IMAGE_POSE, sess, img,
				       out_imgname, len_img, len_out_imgname);
}

int vaccel_image_pose_unpack(struct vaccel_session *sess,
			     struct vaccel_arg *read, int nr_read,
			     struct vaccel_arg *write, int nr_write)
{
	return vaccel_image_op_unpack(VACCEL_OP_IMAGE_POSE, sess, read, nr_read,
				      1, write, nr_write, 1);
}

int vaccel_image_depth(struct vaccel_session *sess, const void *img,
		       unsigned char *out_imgname, size_t len_img,
		       size_t len_out_imgname)
{
	return vaccel_image_op_no_text(VACCEL_OP_IMAGE_DEPTH, sess, img,
				       out_imgname, len_img, len_out_imgname);
}

int vaccel_image_depth_unpack(struct vaccel_session *sess,
			      struct vaccel_arg *read, int nr_read,
			      struct vaccel_arg *write, int nr_write)
{
	return vaccel_image_op_unpack(VACCEL_OP_IMAGE_DEPTH, sess, read,
				      nr_read, 1, write, nr_write, 1);
}

__attribute__((constructor)) static void vaccel_ops_init(void)
{
}

__attribute__((destructor)) static void vaccel_ops_fini(void)
{
	vaccel_prof_region_print(&image_op_stats);
}
