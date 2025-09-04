// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "vaccel/session.h"

#ifdef __cplusplus
extern "C" {
#endif

__attribute__((
	deprecated("The function will be removed in a future release"))) int
vaccel_minmax(struct vaccel_session *sess, const double *indata, int ndata,
	      int low_threshold, int high_threshold, double *outdata,
	      double *min, double *max);

#ifdef __cplusplus
}
#endif
