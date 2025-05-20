// SPDX-License-Identifier: Apache-2.0

#include "vaccel.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#define ELEM_2D(array, i, j, ld) (*((array) + (ptrdiff_t)((i) * (ld)) + (j)))

enum { M = 512, N = 512, K = 512 };

void init(long long int m, long long int n, long long int k, float *A, float *B,
	  float *C)
{
	for (long long int i = 0; i < m; ++i)
		for (long long int j = 0; j < k; ++j)
			ELEM_2D(A, i, j, k) = ((float)i * (float)j) / (float)m;

	for (long long int i = 0; i < k; ++i)
		for (long long int j = 0; j < n; ++j)
			ELEM_2D(B, i, j, n) = ((float)i * (float)j) / (float)m;

	for (long long int i = 0; i < m; ++i)
		for (long long int j = 0; j < n; ++j)
			ELEM_2D(C, i, j, n) = ((float)i * (float)j) / (float)m;
}

int main(int argc, char *argv[])
{
	int ret;
	FILE *data_fp = NULL;
	const float alpha = 32412.0F;
	const float beta = 2123.0F;

	float a[M * K];
	float b[K * N];
	float c[M * N];
	long long int m = M;
	long long int n = N;
	long long int k = K;

	init(m, n, k, a, b, c);

	struct vaccel_session sess;
	struct vaccel_prof_region sgemm_stats =
		VACCEL_PROF_REGION_INIT("sgemm");

	if (argc > 3) {
		fprintf(stderr, "Usage: %s [iterations] [out_file]\n", argv[0]);
		return VACCEL_EINVAL;
	}

	ret = vaccel_session_init(&sess, 0);
	if (ret) {
		fprintf(stderr, "Could not initialize vAccel session\n");
		goto out_close;
	}

	vaccel_op_type_t op_type = VACCEL_OP_BLAS_SGEMM;
	struct vaccel_arg read[] = {
		{ .size = sizeof(vaccel_op_type_t),
		  .buf = &op_type,
		  .argtype = 0 },
		{ .size = sizeof(m), .buf = &m, .argtype = 0 },
		{ .size = sizeof(n), .buf = &n, .argtype = 0 },
		{ .size = sizeof(k), .buf = &k, .argtype = 0 },
		{ .size = sizeof(alpha), .buf = (void *)&alpha, .argtype = 0 },
		{ .size = sizeof(a), .buf = a, .argtype = 0 },
		{ .size = sizeof(k), .buf = &k, .argtype = 0 },
		{ .size = sizeof(b), .buf = b, .argtype = 0 },
		{ .size = sizeof(n), .buf = &n, .argtype = 0 },
		{ .size = sizeof(beta), .buf = (void *)&beta, .argtype = 0 },
		{ .size = sizeof(n), .buf = &n, .argtype = 0 },
	};
	struct vaccel_arg write[] = {
		{ .size = sizeof(c), .buf = c, .argtype = 0 },
	};

	const int iter = (argc > 1) ? atoi(argv[1]) : 1;
	for (int i = 0; i < iter; i++) {
		vaccel_prof_region_start(&sgemm_stats);

		ret = vaccel_genop(&sess, read, sizeof(read) / sizeof(read[0]),
				   write, sizeof(write) / sizeof(write[0]));

		vaccel_prof_region_stop(&sgemm_stats);

		if (ret) {
			fprintf(stderr, "Could not run sgemm\n");
			goto release_session;
		}
	}

	if (argc == 3 && iter > 0) {
		data_fp = fopen(argv[1], "wb");
		if (!data_fp) {
			fprintf(stderr,
				"Could not open file for writing. Will not write results\n");
			goto release_session;
		}

		fwrite(c, sizeof(float), (size_t)m * (size_t)n, data_fp);
	}

release_session:
	if (vaccel_session_release(&sess))
		fprintf(stderr, "Could not release session\n");
out_close:
	if (data_fp)
		fclose(data_fp);

	vaccel_prof_region_print(&sgemm_stats);
	vaccel_prof_region_release(&sgemm_stats);

	return ret;
}
