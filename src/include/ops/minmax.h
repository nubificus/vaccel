// SPDX-License-Identifier: Apache-2.0

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

struct vaccel_session;

int vaccel_minmax(struct vaccel_session *sess, const double *indata, int ndata,
		  int low_threshold, int high_threshold, double *outdata,
		  double *min, double *max);

#ifdef __cplusplus
}
#endif
