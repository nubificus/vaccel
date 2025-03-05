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

	init(M, N, K, a, b, c);

	struct vaccel_session session;
	struct vaccel_prof_region sgemm_stats =
		VACCEL_PROF_REGION_INIT("sgemm");

	if (argc > 3) {
		fprintf(stderr, "Usage: %s [iterations] [out_file]\n", argv[0]);
		return VACCEL_EINVAL;
	}

	ret = vaccel_session_init(&session, 0);
	if (ret) {
		fprintf(stderr, "Could not initialize session\n");
		goto out_close;
	}

	const int iter = (argc > 1) ? atoi(argv[1]) : 1;
	for (int i = 0; i < iter; i++) {
		vaccel_prof_region_start(&sgemm_stats);

		ret = vaccel_sgemm(&session, M, N, K, alpha, a, K, b, N, beta,
				   c, N);

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

		fwrite(c, sizeof(float), (size_t)M * (size_t)N, data_fp);
	}

release_session:
	if (vaccel_session_release(&session))
		fprintf(stderr, "Could not release session\n");
out_close:
	if (data_fp)
		fclose(data_fp);

	vaccel_prof_region_print(&sgemm_stats);
	vaccel_prof_region_release(&sgemm_stats);

	return ret;
}
