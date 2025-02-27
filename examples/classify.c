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
	char *image = NULL;
	size_t image_size;
	char out_text[512];
	char out_imagename[512];
	struct vaccel_session sess;
	struct vaccel_resource model = { .id = -1 };

	if (argc < 3 || argc > 4) {
		fprintf(stderr, "Usage: %s filename #iterations [model]\n",
			argv[0]);
		return VACCEL_EINVAL;
	}

	ret = vaccel_session_init(&sess, 0);
	if (ret) {
		fprintf(stderr, "Could not initialize session\n");
		return ret;
	}

	printf("Initialized session with id: %" PRId64 "\n", sess.id);

	if (argc == 4) {
		ret = vaccel_resource_init(&model, argv[3],
					   VACCEL_RESOURCE_MODEL);
		if (ret) {
			fprintf(stderr, "Could not create model resource\n");
			goto release_session;
		}

		ret = vaccel_resource_register(&model, &sess);
		if (ret) {
			fprintf(stderr,
				"Could not register model to session\n");
			goto release_resource;
		}
	}

	ret = fs_file_read(argv[1], (void **)&image, &image_size);
	if (ret)
		goto unregister_resource;

	for (int i = 0; i < atoi(argv[2]); ++i) {
		ret = vaccel_image_classification(
			&sess, image, (unsigned char *)out_text,
			(unsigned char *)out_imagename, image_size,
			sizeof(out_text), sizeof(out_imagename));
		if (ret) {
			fprintf(stderr, "Could not run op: %d\n", ret);
			goto unregister_resource;
		}
	}
	printf("classification tags: %s\n", out_text);

unregister_resource:
	if (model.id > 0 &&
	    vaccel_resource_unregister(&model, &sess) != VACCEL_OK)
		fprintf(stderr, "Could not unregister model from session\n");
release_resource:
	if (model.id > 0 && vaccel_resource_release(&model) != VACCEL_OK)
		fprintf(stderr, "Could not release model\n");

release_session:
	if (vaccel_session_release(&sess) != VACCEL_OK)
		fprintf(stderr, "Could not release session\n");

	if (image)
		free(image);

	return ret;
}
