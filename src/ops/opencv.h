#ifndef __VACCEL_OPENCV_H__
#define __VACCEL_OPENCV_H__

//#pragma once

//struct vaccel_session;
//struct vaccel_arg;

//int vaccel_opencv(struct vaccel_session *sess, struct vaccel_arg *read,
		//int nr_read, struct vaccel_arg *write, int nr_write);



#include <stdint.h>
#include <stddef.h>


#ifdef __cplusplus
extern "C" {
#endif
enum vaccel_op_type {
	VACCEL_NO_OP=0,
	VACCEL_OPENCV_OPTFLOW,							/* 1 */
	VACCEL_OPENCV_FIND_CHESSBOARD_CORNERS,			/* 2 */
	VACCEL_OPENCV_CORNER_SUB_PIX,					/* 3 */
	VACCEL_OPENCV_CALIBRATE_CAMERA,					/* 4 */
	VACCEL_OPENCV_GET_OPTIMAL_NEW_CAMERA_MATRIX,	/* 5 */
	VACCEL_OPENCV_STEREO_CALIBRATE,					/* 6 */
	VACCEL_OPENCV_STEREO_RECTIFY,					/* 7 */
	VACCEL_OPENCV_INIT_UNDISTORT_RECTIFY_MAP,		/* 8 */
	VACCEL_OPENCV_REMAP,							/* 9 */
	VACCEL_OPENCV_IN_RANGE,							/* 10 */
	VACCEL_OPENCV_FIND_CONTOURS,					/* 11 */
	VACCEL_OPENCV_SOLVE,							/* 12 */
	VACCEL_OPENCV_BM_CREATE_COMPUTE,				/* 13 */
	VACCEL_OPENCV_SGBM_CREATE_COMPUTE				/* 14 */
};

static const char *vaccel_op_name[] = {
	"OpenCV optflow",
	"OpenCV find chessboard corners",
	"OpenCV corner sub pix",
	"OpenCV calibrate camera",
	"OpenCV get optimal new camera matrix",
	"OpenCV stereo calibrate",
	"OpenCV stereo rectify",
	"OpenCV init undistort rectify map",
	"OpenCV remap",
	"OpenCV in range",
	"OpenCV find contours",
	"OpenCV solve",
	"OpenCV stereoBM",
	"OpenCV stereoSGBM"
};

static inline const char *vaccel_op_type_str(enum vaccel_op_type op_type)
{
	return vaccel_op_name[op_type];
}

// int vaccel_opencv_unpack(struct vaccel_session *sess, struct vaccel_arg *read,
// 		int nr_read, struct vaccel_arg *write, int nr_write);

#ifdef __cplusplus
}
#endif

#endif /* __VACCEL_OPENCV_H__ */

