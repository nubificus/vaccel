#include <vaccel.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <errno.h>
#include <time.h>

#define M 512
#define N 512
#define K 512

#define ELEM_2D(array, i, j, ld) (*((array) + i*ld + j))

#define timespec_usec(t) ((double)(t).tv_nsec / 10e3 + (double)(t).tv_sec * 10e6)
#define time_diff_usec(t0, t1) (timespec_usec((t1)) - timespec_usec((t0)))

void init(int m, int n, int k, float *A, float *B, float *C)
{
	for (int i = 0; i < m; ++i)
		for (int j = 0; j < k; ++j)
			ELEM_2D(A, i, j, k) = ((float) i * j) / m;

	for (int i = 0; i < k; ++i)
		for (int j = 0; j < n; ++j)
			ELEM_2D(B, i, j, n) = ((float) i * j) / m;

	for (int i = 0; i < m; ++i)
		for (int j = 0; j < n; ++j)
			ELEM_2D(C, i, j, n) = ((float) i * j) / m;
}

int main(int argc, char *argv[])
{
	FILE *data_fp = NULL;
	int ret = VACCEL_EINVAL;

	if (argc == 2) {
		data_fp = fopen(argv[1], "wb");
		if (!data_fp)
			fprintf(stderr, "Could not open file for writing. Will not write results\n");
	}

	float alpha = 32412.0, beta = 2123.0;

	//float A[M*K];
	//float B[K*N];
	//float C[M*N];
	float *A, *B, *C;
	int64_t *m, *n, *k;

	A = (float*) malloc(M * K * sizeof(float));
	if (!A) {
		fprintf(stderr, "Could not allocate memory for Input A matrix\n");
		ret = VACCEL_ENOMEM;
		goto free_out;
	}
	B = (float*) malloc(K * N * sizeof(float));
	if (!B) {
		fprintf(stderr, "Could not allocate memory for target matrix\n");
		ret = VACCEL_ENOMEM;
		goto free_out_1;
	}

	C = (float*) malloc(M * N * sizeof(float));
	if (!C) {
		fprintf(stderr, "Could not allocate memory for target matrix\n");
		ret = VACCEL_ENOMEM;
		goto free_out_2;
	}

	m = malloc(sizeof(int64_t));
	n = malloc(sizeof(int64_t));
	k = malloc(sizeof(int64_t));

	*m = M;
	*n = N;
	*k = K;

	init(M, N, K, A, B, C);

	struct vaccel_session session;

	ret = vaccel_sess_init(&session, 0);
	if (ret) {
		fprintf(stderr, "Could not initialize vAccel session\n");
		return ret;
	}

	struct timespec t0, t1;

	enum vaccel_op_type op_type = VACCEL_BLAS_SGEMM;
	struct vaccel_arg read[8] = {
		{ .size = sizeof(enum vaccel_op_type), .buf = &op_type},
		{ .size = sizeof(int), .buf = m},
		{ .size = sizeof(int), .buf = n},
		{ .size = sizeof(int), .buf = k},
		{ .size = sizeof(float), .buf = &alpha},
		{ .size = sizeof(float) * M * K, .buf = A},
		{ .size = sizeof(float) * N * K, .buf = B},
		{ .size = sizeof(float), .buf = &beta},
	};
	struct vaccel_arg write[1] = {
		{ .size = sizeof(float) * M * N, .buf = C},
	};

	clock_gettime(CLOCK_MONOTONIC_RAW, &t0);
	ret = vaccel_genop(&session, &read[0], 8, &write[0], 1);
	if (ret) {
		fprintf(stderr, "Could not run sgemm\n");
		goto out;
	}
	clock_gettime(CLOCK_MONOTONIC_RAW, &t1);

	fprintf(stdout, "Execution time: %lf msec\n",
			time_diff_usec(t0, t1) / 10e3);
	if (data_fp)
		fwrite(C, sizeof(float), M * N, data_fp);

out:
	vaccel_sess_free(&session);
	free(C);
free_out_2:
	free(A);
free_out_1:
	free(B);
free_out:
	return ret;
}
