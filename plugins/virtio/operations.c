#include <vaccel.h>
#include <stddef.h>
#include <sys/ioctl.h>
#include <accel.h>

#include "ioctl.h"
#include "log.h"
#include "session.h"

int virtio_noop(struct vaccel_session *sess)
{
	unsigned int op_type = VACCEL_NO_OP;
	struct accel_session vsess = { 0 };
	struct accel_arg arg = { sizeof(op_type), (char *)&op_type };

	vsess.id = sess->session_id;
	vsess.op.out_nr = 1;
	vsess.op.out = &arg;

	vaccel_debug("[virtio] session:%u Executing noop",
			sess->session_id);

	return dev_write(VACCEL_DO_OP, &vsess);
}

int virtio_sgemm(struct vaccel_session *sess, uint32_t k, uint32_t m,
		uint32_t n, size_t len_a, size_t len_b, size_t len_c,
		float *a, float *b, float *c)
{
	unsigned int op_type = VACCEL_BLAS_SGEMM;
	struct accel_session vsess = { 0 };
	struct accel_arg args[7] = {
		{ sizeof(op_type), (char *)&op_type },
		{ sizeof(k), (char *)&k },
		{ sizeof(m), (char *)&m },
		{ sizeof(n), (char *)&n },
		{ len_a, (unsigned char *)a },
		{ len_b, (unsigned char *)b },
		{ len_c, (unsigned char *)c },
	};

	vsess.id = sess->session_id;
	vsess.op.out_nr = 6;
	vsess.op.out = args;
	vsess.op.in_nr = 1;
	vsess.op.in = &args[6];

	vaccel_debug("[virtio] session:%u Executing sgemm",
			sess->session_id);

	return dev_write(VACCEL_DO_OP, &vsess);
}

int virtio_image_classification(struct vaccel_session *sess, void *img,
		char *out_text, char *out_imgname,
		size_t len_img, size_t len_out_text, size_t len_out_imgname)
{
	unsigned int op_type = VACCEL_IMG_CLASS;
	struct accel_session vsess = { 0 };
	struct accel_arg args[4] = {
		{ sizeof(op_type), (char *)&op_type },
		{ len_img, img },
		{ len_out_text, out_text },
		{ len_out_imgname, out_imgname },
	};

	vsess.id = sess->session_id;
	vsess.op.out_nr = 2;
	vsess.op.out = &args[0];
	vsess.op.in_nr = 2;
	vsess.op.in = &args[2];

	vaccel_debug("[virtio] session:%u Executing image classification",
			sess->session_id);

	return dev_write(VACCEL_DO_OP, &vsess);
}

int virtio_image_detection(struct vaccel_session *sess, void *img,
		char *out_imgname, size_t len_img, size_t len_out_imgname)
{
	unsigned int op_type = VACCEL_IMG_DETEC;
	struct accel_session vsess = { 0 };
	struct accel_arg args[3] = {
		{ sizeof(op_type), (char *)&op_type },
		{ len_img, img },
		{ len_out_imgname, out_imgname },
	};

	vsess.id = sess->session_id;
	vsess.op.out_nr = 2;
	vsess.op.out = &args[0];
	vsess.op.in_nr = 1;
	vsess.op.in = &args[2];

	vaccel_debug("[virtio] session:%u Executing image detection",
			sess->session_id);

	return dev_write(VACCEL_DO_OP, &vsess);
}

int virtio_image_segmentation(struct vaccel_session *sess, void *img,
		char *out_imgname, size_t len_img, size_t len_out_imgname)
{
	unsigned int op_type = VACCEL_IMG_SEGME;
	struct accel_session vsess = { 0 };
	struct accel_arg args[3] = {
		{ sizeof(op_type), (char *)&op_type },
		{ len_img, img },
		{ len_out_imgname, out_imgname },
	};

	vsess.id = sess->session_id;
	vsess.op.out_nr = 2;
	vsess.op.out = &args[0];
	vsess.op.in_nr = 1;
	vsess.op.in = &args[2];

	vaccel_debug("[virtio] session:%u Executing image segmentation",
			sess->session_id);

	return dev_write(VACCEL_DO_OP, &vsess);
}
