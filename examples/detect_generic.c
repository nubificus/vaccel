// SPDX-License-Identifier: Apache-2.0

#include "common/inference_helpers.h"
#include "utils/fs.h"
#include "vaccel.h"
#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { STR_SIZE_MAX = 512 };

static void print_detections(const unsigned char *raw, char **labels,
			     size_t nr_labels)
{
	if (!labels || nr_labels == 0) {
		printf("detection imagename: %s\n", raw);
		return;
	}

	char buf[STR_SIZE_MAX];
	strncpy(buf, (const char *)raw, sizeof(buf) - 1);
	buf[sizeof(buf) - 1] = '\0';

	printf("detections:\n");
	char *saveptr = NULL;
	for (char *det = strtok_r(buf, ";", &saveptr); det != NULL;
	     det = strtok_r(NULL, ";", &saveptr)) {
		while (*det == ' ')
			det++;
		char *space = strchr(det, ' ');
		if (space == NULL) {
			printf("  %s\n", det);
			continue;
		}
		*space = '\0';
		const char *name =
			inference_resolve_label(det, labels, nr_labels);
		printf("  %s %s\n", name, space + 1);
	}
}

int main(int argc, char *argv[])
{
	int ret;
	char *image = NULL;
	size_t image_size;
	unsigned char out_imagename[STR_SIZE_MAX] = { '\0' };
	struct vaccel_session sess;
	struct vaccel_resource model = { .id = 0 };
	char **labels = NULL;
	size_t nr_labels = 0;
	struct vaccel_profiler_region detect_stats =
		VACCEL_PROFILER_REGION_INIT("detect");

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

	struct vaccel_arg_array read_args;
	ret = vaccel_arg_array_init(&read_args, 2);
	if (ret) {
		fprintf(stderr, "Could not initialize read args array\n");
		goto free_labels;
	}

	struct vaccel_arg_array write_args;
	ret = vaccel_arg_array_init(&write_args, 1);
	if (ret) {
		fprintf(stderr, "Could not initialize write args array\n");
		goto release_read_args_array;
	}

	const uint8_t op_type = (uint8_t)VACCEL_OP_IMAGE_DETECT;
	ret = vaccel_arg_array_add_uint8(&read_args, (uint8_t *)&op_type);
	if (ret) {
		fprintf(stderr, "Failed to pack op_type arg\n");
		goto release_write_args_array;
	}
	ret = vaccel_arg_array_add_buffer(&read_args, image, image_size);
	if (ret) {
		fprintf(stderr, "Failed to pack image arg\n");
		goto release_write_args_array;
	}

	ret = vaccel_arg_array_add_uchar_array(
		&write_args, out_imagename,
		sizeof(out_imagename) / sizeof(out_imagename[0]));
	if (ret) {
		fprintf(stderr, "Failed to pack out_imagename arg\n");
		goto release_write_args_array;
	}

	const int iter = (argc > 2) ? atoi(argv[2]) : 1;
	for (int i = 0; i < iter; i++) {
		vaccel_profiler_region_start(&detect_stats);

		ret = vaccel_genop(&sess, read_args.args, read_args.count,
				   write_args.args, write_args.count);

		vaccel_profiler_region_stop(&detect_stats);

		if (ret) {
			fprintf(stderr, "Could not run op: %d\n", ret);
			goto unregister_resource;
		}

		print_detections(out_imagename, labels, nr_labels);
	}

release_write_args_array:
	if (vaccel_arg_array_release(&write_args))
		fprintf(stderr, "Could not release write args array\n");
release_read_args_array:
	if (vaccel_arg_array_release(&read_args))
		fprintf(stderr, "Could not release read args array\n");
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

	vaccel_profiler_region_print(&detect_stats);
	vaccel_profiler_region_release(&detect_stats);

	return ret;
}
