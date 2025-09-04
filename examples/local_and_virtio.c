// SPDX-License-Identifier: Apache-2.0

#include "vaccel.h"
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { INPUT_VAL = 10 };

int main(int argc, char *argv[])
{
	int ret;
	int input = INPUT_VAL;
	int output = 0;
	struct vaccel_session virtio_sess;
	struct vaccel_session local_sess;
	struct vaccel_session *sess;
	struct vaccel_resource object;
	struct vaccel_prof_region mytestfunc_stats =
		VACCEL_PROF_REGION_INIT("mytestfunc");

	if (argc < 2 || argc > 3) {
		fprintf(stderr, "Usage: %s <lib_file> [iterations]\n", argv[0]);
		return VACCEL_EINVAL;
	}

	ret = vaccel_resource_init(&object, argv[1], VACCEL_RESOURCE_LIB);
	if (ret) {
		fprintf(stderr, "Could not initialize resource: %s\n",
			strerror(ret));
		return ret;
	}

	if (vaccel_session_init(&virtio_sess, VACCEL_PLUGIN_REMOTE)) {
		fprintf(stderr, "Could not initialize virtio session\n");
		goto release_resource;
	}

	printf("Initialized virtio session with id: %" PRId64 "\n",
	       virtio_sess.id);

	if (vaccel_session_init(&local_sess, 0)) {
		fprintf(stderr, "Could not initialize local session\n");
		goto release_virtio_session;
	}

	printf("Initialized local session with id: %" PRId64 "\n",
	       local_sess.id);

	if (vaccel_resource_register(&object, &local_sess)) {
		fprintf(stderr,
			"Could not register resource with local session\n");
		goto release_local_session;
	}

	if (vaccel_resource_register(&object, &virtio_sess)) {
		fprintf(stderr,
			"Could not register resource with virtio session\n");
		goto unregister_local_resource;
	}

	struct vaccel_arg read[] = {
		{ .size = sizeof(input), .buf = &input, .type = VACCEL_ARG_RAW }
	};
	struct vaccel_arg write[] = {
		{ .size = sizeof(output),
		  .buf = &output,
		  .type = VACCEL_ARG_RAW },
	};

	const int iter = (argc > 2) ? atoi(argv[2]) : 1;
	for (int i = 0; i < iter; i++) {
		sess = i % 2 ? &local_sess : &virtio_sess;

		vaccel_prof_region_start(&mytestfunc_stats);

		ret = vaccel_exec_with_resource(
			sess, &object, "mytestfunc", read,
			sizeof(read) / sizeof(read[0]), write,
			sizeof(write) / sizeof(write[0]));

		vaccel_prof_region_stop(&mytestfunc_stats);

		if (ret) {
			fprintf(stderr, "Could not run op: %d\n", ret);
			goto unregister_virtio_resource;
		}

		printf("output(2x%d): %d\n", input, output);
	}

unregister_virtio_resource:
	if (vaccel_resource_unregister(&object, &virtio_sess))
		fprintf(stderr,
			"Could not unregister resource from virtio session\n");
unregister_local_resource:
	if (vaccel_resource_unregister(&object, &local_sess))
		fprintf(stderr,
			"Could not unregister resource from local session\n");
release_local_session:
	if (vaccel_session_release(&local_sess))
		fprintf(stderr, "Could not release local session\n");
release_virtio_session:
	if (vaccel_session_release(&virtio_sess))
		fprintf(stderr, "Could not release virtio session\n");
release_resource:
	if (vaccel_resource_release(&object))
		fprintf(stderr, "Could not release resource\n");

	vaccel_prof_region_print(&mytestfunc_stats);
	vaccel_prof_region_release(&mytestfunc_stats);

	return ret;
}
