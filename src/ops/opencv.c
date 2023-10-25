#include "error.h"
#include "plugin.h"
#include "log.h"
#include "vaccel_ops.h"
#include "genop.h"
#include "session.h"
#include "vaccel_prof.h"
#include "opencv.h"

typedef struct {
    int x;
    int y;
} Point2i;

typedef struct {
	size_t size;
    Point2i* data;
} Point2iVector;

typedef struct {
	size_t size;
    Point2iVector* data;
} VectorOfPoint2iVector;

typedef struct {
    int x;
    int y;
	int z;
	int w;
} Point4i;

typedef struct {
	size_t size;
    Point4i* data;
} Point4iVector;

typedef struct {
	size_t size;
    Point4iVector* data;
} VectorOfPoint4iVector;

typedef struct {
    float x;
    float y;
} Point2f;

typedef struct {
	size_t size;
    Point2f* data;
} Point2fVector;

typedef struct {
	size_t size;
    Point2fVector *data;
} VectorOfPoint2fVector;

typedef struct {
    float x;
    float y;
	float z;
} Point3f;

typedef struct {
	size_t size;
    Point3f* data;
} Point3fVector;

typedef struct {
	size_t size;
    Point3fVector *data;
} VectorOfPoint3fVector;

typedef struct {
    int width;
    int height;
} Size;

typedef struct {
    int rows;
    int columns;
	float* data;
} Mat;

typedef struct {
	size_t size;
    unsigned char* data;
} UcharVector;

typedef struct {
	size_t size;
    float* data;
} FloatVector;

typedef struct {
    int type;
    int maxCount;
    double epsilon;
} TermCriteria;

struct vaccel_prof_region opencv_op_stats =
	VACCEL_PROF_REGION_INIT("vaccel_opencv_op");

int vaccel_opencv(enum vaccel_op_type op_type, struct vaccel_session *sess)
{
	int ret;

	if (!sess)
		return VACCEL_EINVAL;

	vaccel_debug("session:%u Looking for plugin implementing %s",
			sess->session_id, vaccel_op_type_str(op_type));

	vaccel_prof_region_start(&opencv_op_stats);

	//Get implementation
	int (*plugin_op)() = get_plugin_op(op_type, sess->hint);
	if (!plugin_op)
		return VACCEL_ENOTSUP;
		goto out;

	ret = plugin_op(sess, op_type);

out:
	vaccel_prof_region_stop(&opencv_op_stats);
	return ret;
}

#define vaccel_opencv_op_no_text(op_type, sess) \
		vaccel_opencv_op(op_type, sess)

int vaccel_opencv_unpack(enum vaccel_op_type op_type,
		struct vaccel_session *sess, struct vaccel_arg *read,
		int nr_read, int nr_read_req, struct vaccel_arg *write,
		int nr_write, int nr_write_req)
{
//TODO: sanity check args
#if 1
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
#endif

	return vaccel_opencv(op_type,sess);
}

void vaccel_calcOpticalFlowPyrLK(struct vaccel_session *session, Mat prevFrame, Mat currentFrame,
			Point2fVector prevPoints, Point2fVector *currentPoints, UcharVector *status,
			FloatVector *err, Size winSize, int maxLevel, TermCriteria criteria)
{
	return vaccel_opencv_op(VACCEL_OPENCV_OPTFLOW, session);
}


bool vaccel_findChessboardCorners(struct vaccel_session *session, Mat image,
							Size pattern_size, Point2fVector *corners)
{
	return vaccel_opencv_op(VACCEL_OPENCV_FIND_CHESSBOARD_CORNERS, session);
}


void vaccel_calibrateCamera(struct vaccel_session *session, VectorOfPoint3fVector *objpoints, 
						VectorOfPoint2fVector *imgpoints, Size *imagesize,
	 					Mat *cameraMatrix, Mat *distCoeffs, Mat *R, Mat *T)
{
	return vaccel_opencv_op(VACCEL_OPENCV_CALIBRATE_CAMERA, session);
}


cv::Mat vaccel_getOptimalNewCameraMatrix(struct vaccel_session *session, Mat cameraMatrix, Mat distCoeffs,
                                       Size imgSize, double alpha)
{
	return vaccel_opencv_op(VACCEL_OPENCV_GET_OPTIMAL_NEW_CAMERA_MATRIX, session);
}


void  vaccel_stereoCalibrate(struct vaccel_session *session, VectorOfPoint3fVector *objpoints,
									VectorOfPoint2fVector *imgpointsL, VectorOfPoint2fVector *imgpointsR,
									Mat *new_mtxL, Mat *distL, Mat *new_mtxR, Mat *distR, Size *imagesizeR,
									void *Rot, void *Trns, void *Emat, void *Fmat,
									int flag, TermCriteria criteria)
{
	return vaccel_opencv_op(VACCEL_OPENCV_STEREO_CALIBRATE, session);
}


void vaccel_stereoRectify(struct vaccel_session *session, Mat *new_mtxL, Mat *distL, Mat *new_mtxR, Mat *distR,
                    				Size *imagesizeR, Mat *Rot, Mat *Trns,
									Mat *rect_l,  Mat *rect_r,  Mat *proj_mat_l,  Mat *proj_mat_r,  Mat *Q, int rec_flag)
{
	return vaccel_opencv_op(VACCEL_OPENCV_STEREO_RECTIFY, session);
}


void vaccel_remap(struct vaccel_session *session,
				Mat *src, Mat *dst, Mat *map1, Mat *map2,
				int interpolation, int border_mode, int border_value)
{
	return vaccel_opencv_op(VACCEL_OPENCV_REMAP, session);
}


void vaccel_inRange(struct vaccel_session *session, Mat depth_map, float lowerb, float upperb, Mat mask)
{
	return vaccel_opencv_op(VACCEL_OPENCV_IN_RANGE, session);
}


void vaccel_findContours(struct vaccel_session *session, Mat *mask, VectorOfPoint2iVector contours, VectorOfPoint4iVector hierarchy, int mode, int method)
{
	return vaccel_opencv_op(VACCEL_OPENCV_FIND_CONTOURS, session);
}


void vaccel_solve(struct vaccel_session *session, Mat *coeff, Mat *Z_mat, Mat *sol, int method)
{
	return vaccel_opencv_op(VACCEL_OPENCV_SOLVE, session);
}


cv::Ptr<cv::StereoBM> vaccel_stereoBM_create_compute(struct vaccel_session *session, int numDisparities, int blockSize, 
													int preFilterType, int preFilterSize, int preFilterCap,
													int textureThreshold,int uniquenessRatio, int speckleRange,
													int speckleWindowSize,int disp12MaxDiff, int minDisparity,
													Mat Left_nice, Mat Right_nice, Mat disp)
{
	return vaccel_opencv_op(VACCEL_OPENCV_BM_CREATE_COMPUTE, session);
}


cv::Ptr<cv::StereoSGBM> vaccel_stereoSGBM_create_compute(struct vaccel_session *session, int minDisparity, int numDisparities,
													int blockSize, int disp12MaxDiff, int uniquenessRatio, int speckleWindowSize,
													int speckleRange, Mat Left_nice, Mat Right_nice, Mat disp)
{
	return vaccel_opencv_op(VACCEL_OPENCV_SGBM_CREATE_COMPUTE, session);
}


__attribute__((constructor))
static void vaccel_tf_ops_init(void)
{
}

__attribute__((destructor))
static void vaccel_tf_ops_fini(void)
{
	vaccel_prof_region_print(&opencv_op_stats);
}