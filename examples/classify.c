// SPDX-License-Identifier: Apache-2.0

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "../src/utils.h"
#include <vaccel.h>


int main(int argc, char *argv[])
{
	int ret;
	char *image;
	size_t image_size;
	char out_text[512];
	char out_imagename[512];
	struct vaccel_shared_object object;

	struct vaccel_session sess;

	if (argc != 4) {
		fprintf(stderr, "Usage: %s filename #iterations model\n", argv[0]);
		return 0;
	}

	ret = vaccel_shared_object_new(&object, argv[3]);
	if (ret) {
		fprintf(stderr, "Could not create shared object resource: %s",
			strerror(ret));
		exit(1);
	}

	ret = vaccel_sess_init(&sess, 0);
	if (ret != VACCEL_OK) {
		fprintf(stderr, "Could not initialize session\n");
		return 1;
	}

	printf("Initialized session with id: %u\n", sess.session_id);

	ret = vaccel_sess_register(&sess, object.resource);
	if (ret) {
		fprintf(stderr, "Could register shared object to session\n");
		exit(1);
	}

	ret = read_file(argv[1], (void **)&image, &image_size);
	if (ret)
		goto close_session;

	for (int i = 0; i < atoi(argv[2]); ++i) {
		struct vaccel_prof_region inference_nested = VACCEL_PROF_REGION_INIT("inference_nested");
		vaccel_prof_region_start(&inference_nested);
		ret = vaccel_image_classification(
			&sess, image, (unsigned char *)out_text,
			(unsigned char *)out_imagename, image_size,
			sizeof(out_text), sizeof(out_imagename));
		vaccel_prof_region_stop(&inference_nested);
		if (ret) {
			fprintf(stderr, "Could not run op: %d\n", ret);
			goto close_session;
		}

		vaccel_prof_region_print(&inference_nested);
		if (i == 0)
			printf("classification tags: %s\n", out_text);
	}

	if (vaccel_sess_unregister(&sess, object.resource) != VACCEL_OK)
		fprintf(stderr, "Could not unregister model with session\n");

close_session:
	free(image);
	if (vaccel_sess_free(&sess) != VACCEL_OK) {
		fprintf(stderr, "Could not clear session\n");
		return 1;
	}
	vaccel_shared_object_destroy(&object);

	return ret;
}
