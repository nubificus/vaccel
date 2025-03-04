// SPDX-License-Identifier: Apache-2.0

#include "vaccel.h"
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
	int ret;
	struct vaccel_session sess;
	struct vaccel_prof_region noop_stats = VACCEL_PROF_REGION_INIT("noop");

	if (argc > 2) {
		fprintf(stderr, "Usage: %s [iterations]\n", argv[0]);
		return VACCEL_EINVAL;
	}

	ret = vaccel_session_init(&sess, 0);
	if (ret) {
		fprintf(stderr, "Could not initialize session\n");
		return ret;
	}

	printf("Initialized session with id: %" PRId64 "\n", sess.id);

	const int iter = (argc > 1) ? atoi(argv[1]) : 1;
	for (int i = 0; i < iter; i++) {
		vaccel_prof_region_start(&noop_stats);

		ret = vaccel_noop(&sess);

		vaccel_prof_region_stop(&noop_stats);

		if (ret) {
			fprintf(stderr, "Could not run op: %d\n", ret);
			goto release_session;
		}
	}

release_session:
	if (vaccel_session_release(&sess))
		fprintf(stderr, "Could not release session\n");

	vaccel_prof_region_print(&noop_stats);
	vaccel_prof_region_release(&noop_stats);

	return ret;
}
