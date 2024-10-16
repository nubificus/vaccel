// SPDX-License-Identifier: Apache-2.0

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <vaccel.h>

enum { M = 512, N = 512, K = 512 };

#define ELEM_2D(array, i, j, ld) (*((array) + (i) * (ld) + (j)))

void init(int m, int n, int k, float *A, float *B, float *C)
{
	for (int i = 0; i < m; ++i)
		for (int j = 0; j < k; ++j)
			ELEM_2D(A, i, j, k) = ((float)i * j) / m;

	for (int i = 0; i < k; ++i)
		for (int j = 0; j < n; ++j)
			ELEM_2D(B, i, j, n) = ((float)i * j) / m;

	for (int i = 0; i < m; ++i)
		for (int j = 0; j < n; ++j)
			ELEM_2D(C, i, j, n) = ((float)i * j) / m;
}

int main(int argc, char *argv[])
{
	FILE *data_fp = NULL;

	if (argc == 2) {
		data_fp = fopen(argv[1], "wb");
		if (!data_fp)
			fprintf(stderr,
				"Could not open file for writing. Will not write results\n");
	}

	float alpha = 32412.0;
	float beta = 2123.0;

	float A[M * K];
	float B[K * N];
	float C[M * N];

	init(M, N, K, A, B, C);

	struct vaccel_session session;

	int ret = vaccel_session_init(&session, 0);
	if (ret)
		goto out_close;

	ret = vaccel_sgemm(&session, M, N, K, alpha, (float *)A, K, (float *)B,
			   N, beta, (float *)C, N);
	if (ret) {
		fprintf(stderr, "Could not run sgemm\n");
		goto out;
	}

	if (data_fp)
		fwrite(C, sizeof(float), (unsigned long)(M * N), data_fp);

out:
	vaccel_session_free(&session);
out_close:
	if (data_fp)
		fclose(data_fp);

	return ret;
}
