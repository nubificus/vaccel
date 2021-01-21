#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "vaccel.h"

extern "C" int coral_classify(char* model_path, char* labels_path, void *img, float thres, char** out_text, size_t *out_len);

using namespace std;

#define IMAGENET_NETWORKS_DEFAULT_PATH "/usr/local/share/imagenet-models/networks"
#define IMAGENET_NETWORKS_ENVVAR "VACCEL_IMAGENET_NETWORKS"

/* Check if a path exists and is a directory */
static bool directory_exists(const char *path)
{
	struct stat s;
	return (!stat(path, &s) && (s.st_mode & S_IFDIR));
}

static const char *find_imagenet_models_path(void)
{
	/* Check first the environment variable */
	char *networks = getenv(IMAGENET_NETWORKS_ENVVAR);
	if (networks && directory_exists(networks))
		return networks;

	if (directory_exists(IMAGENET_NETWORKS_DEFAULT_PATH))
		return IMAGENET_NETWORKS_DEFAULT_PATH;

	return NULL;
}

int coral_image_classification(struct vaccel_session *sess, void *img,
		char *out_text, char *out_imgname,
		size_t len_img, size_t len_out_text, size_t len_out_imgname)
{

	char *out_local_text;
	coral_classify("mobilenet_v1_1.0_224_quant_edgetpu.tflite", "imagenet_labels.txt", img, 0.0001, &out_local_text, &len_out_text);
	snprintf(out_text, len_out_text + 1, "%s", out_local_text);


	return VACCEL_OK;
}
