// SPDX-License-Identifier: Apache-2.0

#include "utils/fs.h"
#include "vaccel.h"
#include <inttypes.h>
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
	struct vaccel_prof_region classify_stats =
		VACCEL_PROF_REGION_INIT("classify");

	if (argc < 2 || argc > 4) {
		fprintf(stderr,
			"Usage: %s <image_file> [iterations] [model_path]\n",
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

	struct vaccel_arg_array read_args;
	ret = vaccel_arg_array_init(&read_args, 2);
	if (ret) {
		fprintf(stderr, "Could not initialize read args array\n");
		goto unregister_resource;
	}

	struct vaccel_arg_array write_args;
	ret = vaccel_arg_array_init(&write_args, 2);
	if (ret) {
		fprintf(stderr, "Could not initialize write args array\n");
		goto release_read_args_array;
	}

	const uint8_t op_type = (uint8_t)VACCEL_OP_IMAGE_CLASSIFY;
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

	ret = vaccel_arg_array_add_uchar_array(
		&write_args, out_text, sizeof(out_text) / sizeof(out_text[0]));
	if (ret) {
		fprintf(stderr, "Failed to pack out_text arg\n");
		goto release_write_args_array;
	}

	const int iter = (argc > 2) ? atoi(argv[2]) : 1;
	for (int i = 0; i < iter; i++) {
		vaccel_prof_region_start(&classify_stats);

		ret = vaccel_genop(&sess, read_args.args, read_args.count,
				   write_args.args, write_args.count);

		vaccel_prof_region_stop(&classify_stats);

		if (ret) {
			fprintf(stderr, "Could not run op: %d\n", ret);
			goto unregister_resource;
		}

		printf("classification tags: %s\n", out_text);
		printf("classification imagename: %s\n", out_imagename);
	}

release_write_args_array:
	if (vaccel_arg_array_release(&write_args))
		fprintf(stderr, "Could not release write args array\n");
release_read_args_array:
	if (vaccel_arg_array_release(&read_args))
		fprintf(stderr, "Could not release read args array\n");
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

	vaccel_prof_region_print(&classify_stats);
	vaccel_prof_region_release(&classify_stats);

	return ret;
}
