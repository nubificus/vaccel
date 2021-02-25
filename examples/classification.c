#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <vaccel.h>

int read_file(const char *filename, char **img, size_t *img_size)
{
	int fd;

	fd = open(filename, O_RDONLY);
	if (fd < 0) {
		perror("open: ");
		return 1;
	}

	struct stat info;
	fstat(fd, &info);
	fprintf(stdout, "Image size: %luB\n", info.st_size);

	char *buf = malloc(info.st_size);
	if (!buf) {
		fprintf(stderr, "Could not allocate memory for image\n");
		goto close_file;
	}

	long bytes = 0;
	do {
		int ret = read(fd, buf, info.st_size);
		if (ret < 0) {
			perror("Error while reading image: ");
			goto free_buff;
		}
		bytes += ret;
	} while (bytes < info.st_size);

	if (bytes < info.st_size) {
		fprintf(stderr, "Could not read image\n");
		goto free_buff;
	}

	*img = buf;
	*img_size = info.st_size;
	close(fd);

	return 0;

free_buff:
	free(buf);
close_file:
	close(fd);
	return 1;
}

int main(int argc, char *argv[])
{
	int ret;
	char *image;
       	size_t image_size;
	char out_text[512];
	struct vaccel_session sess;
	struct vaccel_ml_caffe_model model;

	if (argc != 6) {
		fprintf(stderr, "Usage: %s image model_prototxt model_bin classes #iterations\n", argv[0]);
		return 0;
	}

	ret = vaccel_sess_init(&sess, 0);
	if (ret != VACCEL_OK) {
		fprintf(stderr, "Could not initialize session\n");
		return 1;
	}

	printf("Initialized session with id: %u\n", sess.session_id);

	ret = vaccel_ml_caffe_model_init(&model, 0, argv[2], argv[3], argv[4]);
	if (ret != VACCEL_OK) {
		fprintf(stderr, "could not initialize Caffe model\n");
		goto close_session;
	}

	ret = vaccel_ml_model_register(&sess, VACCEL_ML_CAFFE_MODEL, (struct vaccel_ml_model *)&model);
	if (ret != VACCEL_OK) {
		fprintf(stderr, "Could not register Caffe model with session %u\n", sess.session_id);
		goto close_session;
	}

	ret = read_file(argv[1], &image, &image_size);
	if (ret) {
		fprintf(stderr, "Could not read image file");
		goto close_session;
	}

	for (int i = 0; i < atoi(argv[5]); ++i) {
		ret = vaccel_image_classification(&sess, &model, image,
				image_size, (unsigned char*)out_text,
				sizeof(out_text));
		if (ret) {
			fprintf(stderr, "Could not run op: %d\n", ret);
			goto free_image;
		}

		if (i == 0)
			printf("classification tags: %s\n", out_text);
	}

free_image:
	free(image);
close_session:
	if (vaccel_sess_free(&sess) != VACCEL_OK) {
		fprintf(stderr, "Could not clear session\n");
		return 1;
	}

	return ret;
}
