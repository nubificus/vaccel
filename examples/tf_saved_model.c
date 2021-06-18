#include <vaccel.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

static unsigned char *read_file(const char *path, size_t *len)
{
	struct stat stat;
	int status, fd;

	fd = open(path, O_RDONLY);
	if (fd < 0) {
		perror("Could not open file");
		return NULL;
	}

	status = fstat(fd, &stat);
	if (status < 0) {
		perror("Coult not stat file");
		return NULL;
	}

	unsigned char *ptr = mmap(NULL, stat.st_size, PROT_READ|PROT_WRITE,
				MAP_PRIVATE, fd, 0);
	close(fd);

	if(!ptr)
		return NULL;

	*len = stat.st_size;

	return ptr;
}

int main(int argc, char *argv[])
{
	if (argc != 2) {
		fprintf(stderr, "usage: %s saved_model_path\n", argv[0]);
		return 0;
	}

	struct vaccel_tf_saved_model *model1 = vaccel_tf_saved_model_new();
	if (!model1) {
		fprintf(stderr, "Could not create model\n");
		return 1;
	}

	int ret = vaccel_tf_saved_model_set_path(model1, argv[1]);
	if (ret) {
		fprintf(stderr, "Could not load saved model\n");
		return ret;
	}

	ret = vaccel_tf_saved_model_register(model1);
	if (ret) {
		fprintf(stderr, "Could not create model resource\n");
		return ret;
	}

	struct vaccel_tf_saved_model *model2 = vaccel_tf_saved_model_new();
	if (!model1) {
		fprintf(stderr, "Could not create model\n");
		return 1;
	}

	char fpath[1024];
	size_t len;
	unsigned char *ptr;

	snprintf(fpath, 1024, "%s/saved_model.pb", argv[1]);
	ptr = read_file(fpath, &len);
	if (!ptr) {
		fprintf(stderr, "Could not mmap saved_model.pb\n");
		return 1;
	}

	ret = vaccel_tf_saved_model_set_model(model2, ptr, len);
	if (ret) {
		fprintf(stderr, "Could not set pb file for model\n");
		return ret;
	}

	snprintf(fpath, 1024, "%s/variables/variables.data-00000-of-00001", argv[1]);
	ptr = read_file(fpath, &len);
	if (!ptr) {
		fprintf(stderr, "Could not mmap checkpoint for model\n");
		return 1;
	}

	ret = vaccel_tf_saved_model_set_checkpoint(model2, ptr, len);
	if (ret) {
		fprintf(stderr, "Could not set checkpoint file for model\n");
		return ret;
	}

	snprintf(fpath, 1024, "%s/variables/variables.index", argv[1]);
	ptr = read_file(fpath, &len);
	if (!ptr) {
		fprintf(stderr, "Could not mmap var index for model\n");
		return 1;
	}

	ret = vaccel_tf_saved_model_set_var_index(model2, ptr, len);
	if (ret) {
		fprintf(stderr, "Could not set var index file for model\n");
		return ret;
	}

	ret = vaccel_tf_saved_model_register(model2);
	if (!model2) {
		fprintf(stderr, "Could not create model\n");
		return 1;
	}

	return 0;
}
