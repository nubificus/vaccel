// SPDX-License-Identifier: Apache-2.0

#include "utils/fs.h"
#include "vaccel.h"
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[])
{
	int ret;
	char *file;
	size_t file_size;
	struct vaccel_session sess;
	struct vaccel_prof_region mbench_stats =
		VACCEL_PROF_REGION_INIT("mbench");

	if (argc < 3 || argc > 4) {
		fprintf(stderr, "Usage: %s <time_ms> <lib_file> [iterations]\n",
			argv[0]);
		return VACCEL_EINVAL;
	}

	ret = vaccel_session_init(&sess, 0);
	if (ret) {
		fprintf(stderr, "Could not initialize session\n");
		return ret;
	}

	printf("Initialized session with id: %" PRId64 "\n", sess.id);

	ret = fs_file_read(argv[2], (void **)&file, &file_size);
	if (ret)
		goto release_session;

	struct vaccel_arg read[] = {
		{ .size = sizeof(int), .buf = argv[1] },
		{ .size = file_size, .buf = file },
	};

	const int iter = (argc > 3) ? atoi(argv[3]) : 1;
	for (int i = 0; i < iter; i++) {
		vaccel_prof_region_start(&mbench_stats);

		ret = vaccel_exec(&sess, "mbench", "mbench", read,
				  sizeof(read) / sizeof(read[0]), NULL, 0);

		vaccel_prof_region_stop(&mbench_stats);

		if (ret) {
			fprintf(stderr, "Could not run op: %d\n", ret);
			goto release_session;
		}
	}

release_session:
	free(file);
	if (vaccel_session_release(&sess)) {
		fprintf(stderr, "Could not release session\n");
		return 1;
	}

	vaccel_prof_region_print(&mbench_stats);
	vaccel_prof_region_release(&mbench_stats);

	return ret;
}
