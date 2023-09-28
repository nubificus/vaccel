#ifndef __VACCEL_OPENCV_H__
#define __VACCEL_OPENCV_H__

//#pragma once

//struct vaccel_session;
//struct vaccel_arg;

//int vaccel_optflow(struct vaccel_session *sess, struct vaccel_arg *read,
		//int nr_read, struct vaccel_arg *write, int nr_write);



#include <stdint.h>
#include <stddef.h>

//#include "include/ops/optflow.h"

#ifdef __cplusplus
extern "C" {
#endif

int vaccel_opencv_unpack(struct vaccel_session *sess, struct vaccel_arg *read,
		int nr_read, struct vaccel_arg *write, int nr_write);

#ifdef __cplusplus
}
#endif

#endif /* __VACCEL_OPENCV_H__ */

