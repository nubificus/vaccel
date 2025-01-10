// SPDX-License-Identifier: Apache-2.0

#include "utils/fs.h"
#include "vaccel.h"
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

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

	vaccel_op_type_t op_type = VACCEL_OP_IMAGE_DETECT;
	struct vaccel_arg read[2] = { { .size = sizeof(vaccel_op_type_t),
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
