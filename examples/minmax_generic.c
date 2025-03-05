// SPDX-License-Identifier: Apache-2.0

#include "vaccel.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
	int ret;
	double min;
	double max;
	int low_threshold;
	int high_threshold;
	struct vaccel_prof_region minmax_stats =
		VACCEL_PROF_REGION_INIT("minmax");

	if (argc < 5 || argc > 6) {
		fprintf(stderr,
			"Usage: %s <data_length> <indata_file> <low_threshold> <high_threshold> [iterations]\n",
			argv[0]);
		return VACCEL_EINVAL;
	}

	int ndata = atoi(argv[1]);
	if (ndata <= 0) {
		fprintf(stderr, "Invalid data length: %d\n", ndata);
		return VACCEL_EINVAL;
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
		goto free_in_data;
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

	struct vaccel_session sess;
	ret = vaccel_session_init(&sess, 0);
	if (ret) {
		fprintf(stderr, "Could not initialize vAccel session\n");
		goto free_out;
	}

	vaccel_op_type_t op_type = VACCEL_OP_MINMAX;
	struct vaccel_arg read[] = {
		{ .size = sizeof(vaccel_op_type_t), .buf = &op_type },
		{ .size = ndata * sizeof(double), .buf = indata },
		{ .size = sizeof(int), .buf = &ndata },
		{ .size = sizeof(int), .buf = &low_threshold },
		{ .size = sizeof(int), .buf = &high_threshold },
	};
	struct vaccel_arg write[] = {
		{ .size = sizeof(double) * ndata, .buf = outdata },
		{ .size = sizeof(double), .buf = &min },
		{ .size = sizeof(double), .buf = &max },
	};

	const int iter = (argc > 5) ? atoi(argv[5]) : 1;
	for (int i = 0; i < iter; i++) {
		vaccel_prof_region_start(&minmax_stats);

		ret = vaccel_genop(&sess, read, sizeof(read) / sizeof(read[0]),
				   write, sizeof(write) / sizeof(write[0]));

		vaccel_prof_region_stop(&minmax_stats);

		if (ret) {
			fprintf(stderr, "Could not run kernel: %d\n", ret);
			goto release_session;
		}

		printf("min: %lf max: %lf\n", min, max);
	}

release_session:
	if (vaccel_session_release(&sess))
		fprintf(stderr, "Could not release session\n");
free_out:
	free(outdata);
free_in:
	if (fclose(fp))
		fprintf(stderr, "Could not close input data file\n");
free_in_data:
	free(indata);

	vaccel_prof_region_print(&minmax_stats);
	vaccel_prof_region_release(&minmax_stats);

	return ret;
}
