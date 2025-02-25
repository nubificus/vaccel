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
	char out_text[512];
	char out_imagename[512];
#if VACCEL_EXAMPLES_MODEL_RESOURCE
	struct vaccel_resource model;
#endif
	struct vaccel_session sess;

#if VACCEL_EXAMPLES_MODEL_RESOURCE
	if (argc != 4) {
		fprintf(stderr, "Usage: %s filename #iterations model\n",
			argv[0]);
		return 0;
	}
#else
	if (argc != 3) {
		fprintf(stderr, "Usage: %s filename #iterations\n", argv[0]);
		return 0;
	}
#endif
#if VACCEL_EXAMPLES_MODEL_RESOURCE
	ret = vaccel_resource_init(&model, argv[3], VACCEL_RESOURCE_MODEL);
	if (ret != 0) {
		fprintf(stderr, "Could not create model resource\n");
		return ret;
	}
#endif

	ret = vaccel_session_init(&sess, 0);
	if (ret != VACCEL_OK) {
		fprintf(stderr, "Could not initialize session\n");
		return 1;
	}

	printf("Initialized session with id: %" PRId64 "\n", sess.id);

#if VACCEL_EXAMPLES_MODEL_RESOURCE
	ret = vaccel_resource_register(&model, &sess);
	if (ret) {
		fprintf(stderr, "Could register shared object to session\n");
		exit(1);
	}
#endif
	ret = fs_file_read(argv[1], (void **)&image, &image_size);
	if (ret)
		goto close_session;

	for (int i = 0; i < atoi(argv[2]); ++i) {
#if VACCEL_EXAMPLES_PROFILE
		struct vaccel_prof_region inference_nested =
			VACCEL_PROF_REGION_INIT("inference_nested");
		vaccel_prof_region_start(&inference_nested);
#endif
		ret = vaccel_image_classification(
			&sess, image, (unsigned char *)out_text,
			(unsigned char *)out_imagename, image_size,
			sizeof(out_text), sizeof(out_imagename));
#if VACCEL_EXAMPLES_PROFILE
		vaccel_prof_region_stop(&inference_nested);
#endif
		if (ret) {
			fprintf(stderr, "Could not run op: %d\n", ret);
			goto close_session;
		}

#if VACCEL_EXAMPLES_PROFILE
		vaccel_prof_region_print(&inference_nested);
#endif
		if (i == 0)
			printf("classification tags: %s\n", out_text);
	}

close_session:
#if VACCEL_EXAMPLES_MODEL_RESOURCE
	if (vaccel_resource_unregister(&model, &sess) != VACCEL_OK)
		fprintf(stderr, "Could not unregister model from session\n");
#endif
	free(image);
	if (vaccel_session_release(&sess) != VACCEL_OK) {
		fprintf(stderr, "Could not clear session\n");
		return 1;
	}
#if VACCEL_EXAMPLES_MODEL_RESOURCE
	vaccel_resource_release(&model);
#endif

	return ret;
}
