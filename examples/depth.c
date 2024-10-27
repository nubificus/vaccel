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

	for (int i = 0; i < atoi(argv[2]); ++i) {
		ret = vaccel_image_depth(&sess, image,
					 (unsigned char *)out_imagename,
					 image_size, sizeof(out_imagename));

		if (ret) {
			fprintf(stderr, "Could not run op: %d\n", ret);
			goto close_session;
		}

		if (i == 0)
			printf("depth estimation imagename: %s\n",
			       out_imagename);
	}

close_session:
	free(image);
	if (vaccel_session_release(&sess) != VACCEL_OK) {
		fprintf(stderr, "Could not clear session\n");
		return 1;
	}

	return ret;
}
