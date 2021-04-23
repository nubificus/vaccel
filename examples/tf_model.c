#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>

#include <vaccel.h>

static unsigned char *read_file(const char *path, size_t *len)
{
	struct stat buffer;
	int status, fd;

	fd = open(path, O_RDONLY);
	if (fd < 0) {
		perror("Could not open file");
		return NULL;
	}

	status = fstat(fd, &buffer);
	if (status < 0) {
		perror("Coult not stat file");
		return NULL;
	}

	unsigned char *buff = malloc(buffer.st_size);
	if (!buff) {
		close(fd);
		perror("malloc");
		return NULL;
	}

	size_t bytes = buffer.st_size;
	ssize_t ptr = 0;
	while (bytes) {
		ssize_t ret = read(fd, &buff[ptr], bytes);
		if (ret < 0) {
			perror("read");
			free(buff);
			close(fd);
			return NULL;
		}

		ptr += ret;
		bytes -= ret;
	}

	close(fd);

	*len = ptr;
	return buff;
}

int main(int argc, char *argv[])
{
	if (argc != 2) {
		fprintf(stderr, "usage: %s path_to_tf_model\n", argv[0]);
		return 0;
	}

	struct vaccel_tf_model model;

	int ret = vaccel_tf_model_new(&model, argv[1]);
	if (ret) {
		fprintf(stderr, "Could not create TF model resource: %s",
				strerror(ret));
		exit(1);
	}

	struct vaccel_session sess;
	ret = vaccel_sess_init(&sess, 0);
	if (ret) {
		fprintf(stderr, "Could not create new session\n");
		exit(1);
	}

	ret = vaccel_sess_register(&sess, model.resource);
	if (ret) {
		fprintf(stderr, "Could register model to session\n");
		exit(1);
	}

	struct vaccel_tf_model model2;
	size_t len;
	unsigned char *buff = read_file(argv[1], &len);
	if (!buff) {
		fprintf(stderr, "Could not read model file\n");
		exit(1);
	}
		

	ret = vaccel_tf_model_new_from_buffer(&model2, buff, len);
	if (ret) {
		fprintf(stderr, "Could not create TF model resource from buffer: %s\n",
				strerror(ret));
		exit(1);
	}

	ret = vaccel_sess_register(&sess, model2.resource);
	if (ret) {
		fprintf(stderr, "Could not register model 2 to session\n");
		exit(1);
	}

	ret = vaccel_sess_unregister(&sess, model.resource);
	if (ret) {
		fprintf(stderr, "Could not unregister model from session\n");
		exit(1);
	}

	ret = vaccel_sess_unregister(&sess, model2.resource);
	if (ret) {
		fprintf(stderr, "Could not unregister model 2 from session\n");
		exit(1);
	}
	

	ret = vaccel_tf_model_destroy(&model);
	if (ret) {
		fprintf(stderr, "Could not destroy model\n");
		exit(1);
	}

	ret = vaccel_tf_model_destroy(&model2);
	if (ret) {
		fprintf(stderr, "Could not destroy model2\n");
		exit(1);
	}

	ret = vaccel_sess_free(&sess);
	if (ret) {
		fprintf(stderr, "Could not close session\n");
		exit(1);
	}

	free(buff);

	return 0;
}
