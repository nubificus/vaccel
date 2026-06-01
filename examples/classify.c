// SPDX-License-Identifier: Apache-2.0

#include "common/inference_helpers.h"
#include "utils/fs.h"
#include "vaccel.h"
#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

enum { STR_SIZE_MAX = 512 };

int main(int argc, char *argv[])
{
	int ret;
	char *image = NULL;
	size_t image_size;
	unsigned char out_text[STR_SIZE_MAX] = { '\0' };
	unsigned char out_imagename[STR_SIZE_MAX] = { '\0' };
	struct vaccel_session sess;
	struct vaccel_resource model = { .id = 0 };
	char **labels = NULL;
	size_t nr_labels = 0;
	struct vaccel_profiler_region classify_stats =
		VACCEL_PROFILER_REGION_INIT("classify");

	if (argc < 2 || argc > 5) {
		fprintf(stderr,
			"Usage: %s <image_file> [iterations] [model_path] [labels_path]\n",
			argv[0]);
		return VACCEL_EINVAL;
	}

	ret = vaccel_session_init(&sess, 0);
	if (ret) {
		fprintf(stderr, "Could not initialize session\n");
		return ret;
	}

	printf("Initialized session with id: %" PRId64 "\n", sess.id);

	if (argc >= 4) {
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

	if (argc == 5) {
		ret = inference_load_labels(argv[4], &labels, &nr_labels);
		if (ret) {
			fprintf(stderr, "Could not load labels from %s\n",
				argv[4]);
			goto unregister_resource;
		}
	}

	ret = fs_file_read(argv[1], (void **)&image, &image_size);
	if (ret)
		goto free_labels;

	const int iter = (argc > 2) ? atoi(argv[2]) : 1;
	for (int i = 0; i < iter; i++) {
		vaccel_profiler_region_start(&classify_stats);

		ret = vaccel_image_classification(&sess, image, out_text,
						  out_imagename, image_size,
						  sizeof(out_text),
						  sizeof(out_imagename));

		vaccel_profiler_region_stop(&classify_stats);

		if (ret) {
			fprintf(stderr, "Could not run op: %d\n", ret);
			goto free_labels;
		}

		const char *tag = inference_resolve_label((char *)out_text,
							  labels, nr_labels);
		printf("classification tags: %s\n", tag);
		printf("classification imagename: %s\n", out_imagename);
	}

free_labels:
	if (labels) {
		for (size_t i = 0; i < nr_labels; i++)
			free(labels[i]);
		free(labels);
	}
unregister_resource:
	if (model.id > 0 && vaccel_resource_unregister(&model, &sess))
		fprintf(stderr, "Could not unregister model from session\n");
release_resource:
	if (model.id > 0 && vaccel_resource_release(&model))
		fprintf(stderr, "Could not release model\n");

release_session:
	if (vaccel_session_release(&sess))
		fprintf(stderr, "Could not release session\n");

	if (image)
		free(image);

	vaccel_profiler_region_print(&classify_stats);
	vaccel_profiler_region_release(&classify_stats);

	return ret;
}
