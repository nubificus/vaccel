// SPDX-License-Identifier: Apache-2.0

#include "image.h"
#include "arg.h"
#include "error.h"
#include "log.h"
#include "op.h"
#include "plugin.h"
#include "prof.h"
#include "session.h"
#include "utils/enum.h"
#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>

static struct vaccel_prof_region image_op_stats =
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

	op_debug_plugin_lookup(sess, op_type);

	vaccel_prof_region_start(&image_op_stats);

	int (*plugin_image_op)() = plugin_get_op_func(sess->plugin, op_type);
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
	char op_name[VACCEL_ENUM_STR_MAX];
	vaccel_op_type_name(op_type, op_name, VACCEL_ENUM_STR_MAX);

	if (nr_read != nr_read_req) {
		vaccel_error(
			"Wrong number of read arguments for %s: %d (expected %d)",
			op_name, nr_read, nr_read_req);
		return VACCEL_EINVAL;
	}

	if (nr_write != nr_write_req) {
		vaccel_error(
			"Wrong number of write arguments for %s: %d (expected %d)",
			op_name, nr_write, nr_write_req);
		return VACCEL_EINVAL;
	}

	struct vaccel_arg_array read_args;
	int ret = vaccel_arg_array_wrap(&read_args, read, nr_read);
	if (ret) {
		vaccel_error("Failed to parse %s read args", op_name);
		return VACCEL_EINVAL;
	}

	struct vaccel_arg_array write_args;
	ret = vaccel_arg_array_wrap(&write_args, write, nr_write);
	if (ret) {
		vaccel_error("Failed to parse %s write args", op_name);
		return VACCEL_EINVAL;
	}

	void *img;
	size_t len_img;
	ret = vaccel_arg_array_get_buffer(&read_args, &img, &len_img);
	if (ret) {
		vaccel_error("Failed to unpack img arg for %s", op_name);
		return VACCEL_EINVAL;
	}

	unsigned char *out_imgname = NULL;
	size_t len_out_imgname = 0;
	ret = vaccel_arg_array_get_uchar_array(&write_args, &out_imgname,
					       &len_out_imgname);
	if (ret) {
		vaccel_error("Failed to unpack out_imgname arg for %s",
			     op_name);
		return VACCEL_EINVAL;
	}

	unsigned char *out_text = NULL;
	size_t len_out_text = 0;
	ret = vaccel_arg_array_get_uchar_array(&write_args, &out_text,
					       &len_out_text);
	if (ret && ret != VACCEL_ERANGE) {
		vaccel_error("Failed to unpack out_text arg for %s", op_name);
		return VACCEL_EINVAL;
	}

	return vaccel_image_op(op_type, sess, img, out_text, out_imgname,
			       len_img, len_out_text, len_out_imgname);
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
	vaccel_prof_region_release(&image_op_stats);
}
