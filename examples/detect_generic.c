// SPDX-License-Identifier: Apache-2.0

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "../src/utils/fs.h"
#include <vaccel.h>

int main(int argc, char *argv[])
{
	int ret;
	char *image;
	size_t image_size;
	char out_imagename[512];
	struct vaccel_session sess;

	if (argc != 3) {
		fprintf(stderr, "Usage: %s filename #iterations\n", argv[0]);
		return 0;
	}

	ret = vaccel_session_init(&sess, 0);
	if (ret != VACCEL_OK) {
		fprintf(stderr, "Could not initialize session\n");
		return 1;
	}

	printf("Initialized session with id: %" PRId64 "\n", sess.id);

	ret = fs_file_read(argv[1], (void **)&image, &image_size);
	if (ret)
		goto close_session;

	enum vaccel_op_type op_type = VACCEL_IMG_DETEC;
	struct vaccel_arg read[2] = { { .size = sizeof(enum vaccel_op_type),
					.buf = &op_type },
				      { .size = image_size, .buf = image } };
	struct vaccel_arg write[1] = {
		{ .size = sizeof(out_imagename), .buf = out_imagename },
	};

	for (int i = 0; i < atoi(argv[2]); ++i) {
		ret = vaccel_genop(&sess, read, 2, write, 1);
		if (ret) {
			fprintf(stderr, "Could not run op: %d\n", ret);
			goto close_session;
		}
	}

close_session:
	free(image);
	if (vaccel_session_release(&sess) != VACCEL_OK) {
		fprintf(stderr, "Could not clear session\n");
		return 1;
	}

	return ret;
}
