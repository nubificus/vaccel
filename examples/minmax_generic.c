#include <vaccel.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#define timespec_usec(t) ((double)(t).tv_nsec / 10e3 + (double)(t).tv_sec * 10e6)
#define time_diff_usec(t0, t1) (timespec_usec((t1)) - timespec_usec((t0)))

int main(int argc, char *argv[])
{
	double min, max;
	int low_threshold, high_threshold;
	int ret;

	if (argc != 5) {
		fprintf(stderr, "Usage: %s data_length indata_file low_threshold high_threshold\n",
				argv[0]);
		return 0;
	}

	int ndata = atoi(argv[1]);
	if (ndata <= 0) {
		fprintf(stderr, "Invalid data length: %d\n", ndata);
		return 1;
	}

	double *indata = malloc(ndata * sizeof(double));
	if (!indata) {
		fprintf(stderr, "Could not allocate memory\n");
		return VACCEL_ENOMEM;
	}

	FILE *fp = fopen(argv[2], "r");
	if (!fp) {
		fprintf(stderr, "Could not open input data file\n");
		ret = VACCEL_EIO;
		goto free_in;
	}

	for (int i = 0; i < ndata; ++i) {
		if (fscanf(fp, "%lf\n", &indata[i]) != 1) {
			fprintf(stderr, "Could not read value from file\n");
			ret = VACCEL_EIO;
			goto free_in;
		}
	}

	double *outdata = malloc(ndata * sizeof(double));
	if (!outdata) {
		fprintf(stderr, "Could not allocate memory\n");
		ret = VACCEL_ENOMEM;
		goto free_out;
	}

	low_threshold = atoi(argv[3]);
	high_threshold = atoi(argv[4]);

	struct vaccel_session session;
	ret = vaccel_sess_init(&session, 0);
	if (ret) {
		fprintf(stderr, "Could not initialize vAccel session\n");
		goto free_out;
	}

	struct timespec t0, t1;

	enum vaccel_op_type op_type = VACCEL_MINMAX;
	struct vaccel_arg read[5] = {
		{ .size = sizeof(enum vaccel_op_type), .buf = &op_type},
		{ .size = ndata * sizeof(double), .buf = indata},
		{ .size = sizeof(int), .buf = &ndata},
		{ .size = sizeof(int), .buf = &low_threshold},
		{ .size = sizeof(int), .buf = &high_threshold},
	};
	struct vaccel_arg write[3] = {
		{ .size = sizeof(double)*ndata, .buf = outdata},
		{ .size = sizeof(double), .buf = &min},
		{ .size = sizeof(double), .buf = &max},
	};

	clock_gettime(CLOCK_MONOTONIC_RAW, &t0);
	ret = vaccel_genop(&session, &read[0], 5, &write[0], 3);
	//ret = vaccel_minmax(&session, indata, ndata, low_threshold,
			//high_threshold, outdata, &min, &max);
	clock_gettime(CLOCK_MONOTONIC_RAW, &t1);
	if (ret) {
		fprintf(stderr, "Could not run kernel: %d\n", ret);
		goto close_sess;
	}

	fprintf(stdout, "min: %lf max: %lf Execution time: %lf msec\n",
			min, max, time_diff_usec(t0, t1) / 10e3);

close_sess:
	vaccel_sess_free(&session);
free_out:
	free(outdata);
free_in:
	free(indata);

	return ret;
}
