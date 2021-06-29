#pragma once

struct vaccel_session;

int vaccel_minmax(
	struct vaccel_session *sess,
	const double *indata, int ndata,
	int low_threshold, int high_threshold,
	double *outdata,
	double *min, double *max
);
