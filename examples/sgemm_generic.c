// SPDX-License-Identifier: Apache-2.0

#include "vaccel.h"
#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define ELEM_2D(array, i, j, ld) (*((array) + (ptrdiff_t)((i) * (ld)) + (j)))

enum { M = 512, N = 512, K = 512 };

void init(int64_t m, int64_t n, int64_t k, float *A, float *B, float *C)
{
	for (int64_t i = 0; i < m; ++i)
		for (int64_t j = 0; j < k; ++j)
			ELEM_2D(A, i, j, k) = ((float)i * (float)j) / (float)m;

	for (int64_t i = 0; i < k; ++i)
		for (int64_t j = 0; j < n; ++j)
			ELEM_2D(B, i, j, n) = ((float)i * (float)j) / (float)m;

	for (int64_t i = 0; i < m; ++i)
		for (int64_t j = 0; j < n; ++j)
			ELEM_2D(C, i, j, n) = ((float)i * (float)j) / (float)m;
}

int main(int argc, char *argv[])
{
	int ret;
	FILE *data_fp = NULL;
	float alpha = 32412.0F;
	float beta = 2123.0F;

	float a[M * K];
	float b[K * N];
	float c[M * N];
	int64_t m = M;
	int64_t n = N;
	int64_t k = K;

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
		fprintf(stderr, "Could not initialize session\n");
		goto out_close;
	}

	printf("Initialized session with id: %" PRId64 "\n", sess.id);

	struct vaccel_arg_array read_args;
	ret = vaccel_arg_array_init(&read_args, 11);
	if (ret) {
		fprintf(stderr, "Could not initialize read args array\n");
		goto release_session;
	}

	struct vaccel_arg_array write_args;
	ret = vaccel_arg_array_init(&write_args, 1);
	if (ret) {
		fprintf(stderr, "Could not initialize write args array\n");
		goto release_read_args_array;
	}

	const uint8_t op_type = (uint8_t)VACCEL_OP_BLAS_SGEMM;
	ret = vaccel_arg_array_add_uint8(&read_args, (uint8_t *)&op_type);
	if (ret) {
		fprintf(stderr, "Failed to pack op_type arg\n");
		goto release_write_args_array;
	}

	ret = vaccel_arg_array_add_int64(&read_args, &m);
	if (ret) {
		fprintf(stderr, "Failed to pack m arg\n");
		goto release_write_args_array;
	}

	ret = vaccel_arg_array_add_int64(&read_args, &n);
	if (ret) {
		fprintf(stderr, "Failed to pack n arg\n");
		goto release_write_args_array;
	}

	ret = vaccel_arg_array_add_int64(&read_args, &k);
	if (ret) {
		fprintf(stderr, "Failed to pack k arg\n");
		goto release_write_args_array;
	}

	ret = vaccel_arg_array_add_float(&read_args, &alpha);
	if (ret) {
		fprintf(stderr, "Failed to pack alpha arg\n");
		goto release_write_args_array;
	}

	ret = vaccel_arg_array_add_float_array(&read_args, a,
					       sizeof(a) / sizeof(a[0]));
	if (ret) {
		fprintf(stderr, "Failed to pack a arg\n");
		goto release_write_args_array;
	}

	int64_t lda = k;
	ret = vaccel_arg_array_add_int64(&read_args, &lda);
	if (ret) {
		fprintf(stderr, "Failed to pack lda arg\n");
		goto release_write_args_array;
	}

	ret = vaccel_arg_array_add_float_array(&read_args, b,
					       sizeof(b) / sizeof(b[0]));
	if (ret) {
		fprintf(stderr, "Failed to pack b arg\n");
		goto release_write_args_array;
	}

	int64_t ldb = n;
	ret = vaccel_arg_array_add_int64(&read_args, &ldb);
	if (ret) {
		fprintf(stderr, "Failed to pack ldb arg\n");
		goto release_write_args_array;
	}

	ret = vaccel_arg_array_add_float(&read_args, &beta);
	if (ret) {
		fprintf(stderr, "Failed to pack beta arg\n");
		goto release_write_args_array;
	}

	int64_t ldc = n;
	ret = vaccel_arg_array_add_int64(&read_args, &ldc);
	if (ret) {
		fprintf(stderr, "Failed to pack ldc arg\n");
		goto release_write_args_array;
	}

	ret = vaccel_arg_array_add_float_array(&write_args, c,
					       sizeof(c) / sizeof(c[0]));
	if (ret) {
		fprintf(stderr, "Failed to pack c arg\n");
		goto release_write_args_array;
	}

	const int iter = (argc > 1) ? atoi(argv[1]) : 1;
	for (int i = 0; i < iter; i++) {
		vaccel_prof_region_start(&sgemm_stats);

		ret = vaccel_genop(&sess, read_args.args, read_args.count,
				   write_args.args, write_args.count);

		vaccel_prof_region_stop(&sgemm_stats);

		if (ret) {
			fprintf(stderr, "Could not run sgemm\n");
			goto release_write_args_array;
		}
	}

	if (argc == 3 && iter > 0) {
		data_fp = fopen(argv[1], "wb");
		if (!data_fp) {
			fprintf(stderr,
				"Could not open file for writing. Will not write results\n");
			goto release_write_args_array;
		}

		fwrite(c, sizeof(float), (size_t)m * (size_t)n, data_fp);
	}

release_write_args_array:
	if (vaccel_arg_array_release(&write_args))
		fprintf(stderr, "Could not release write args array\n");
release_read_args_array:
	if (vaccel_arg_array_release(&read_args))
		fprintf(stderr, "Could not release read args array\n");
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
