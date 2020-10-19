#include "vaccel.h"
#include "backend.h"

#include <stdbool.h>
#include <string.h>
#include <dlfcn.h>

/* export supported function as types for the rest of the runtime */
typedef typeof(vaccel_noop) noop_t;
typedef typeof(vaccel_sgemm) sgemm_t;
typedef typeof(vaccel_image_classification) image_classification_t;
typedef typeof(vaccel_image_detection) image_detection_t;
typedef typeof(vaccel_image_segmentation) image_segmentation_t;

int vaccel_noop(struct vaccel_session *sess)
{
	if (!sess)
		return VACCEL_EINVAL;

	//Get implementation
	noop_t *backend_op = get_backend_op(VACCEL_NO_OP);
	if (!backend_op)
		return VACCEL_ENOTSUP;

	return backend_op(sess);
}

int vaccel_image_classification(struct vaccel_session *sess, void *img,
		unsigned char *out_text, unsigned char *out_imgname,
		size_t len_img, size_t len_out_text, size_t len_out_imgname)
{
	if (!sess)
		return VACCEL_EINVAL;

	//Get implementation
	image_classification_t *backend_op = get_backend_op(VACCEL_IMG_CLASS);
	if (!backend_op)
		return VACCEL_ENOTSUP;

	return backend_op(sess, img, out_text, out_imgname, len_img,
			len_out_text, len_out_imgname);
}

int load_backend_plugin(const char *path)
{
	int ret;

	void *dl = dlopen(path, RTLD_LAZY);
	if (!dl)
		return VACCEL_ENOENT;

	int (*init)(struct vaccel_backend *) = dlsym(dl, "vaccel_backend_initialize");
	if (!init)
		return VACCEL_ELIBBAD;

	int (*fini)(struct vaccel_backend *) = dlsym(dl, "vaccel_backend_finalize");
	if (!fini)
		return VACCEL_ELIBBAD;

	struct vaccel_backend *new = malloc(sizeof(*new));
	if (!new) {
		ret = VACCEL_ENOMEM;
		goto close_dl;
	}

	ret = init(new);
	if (ret != VACCEL_OK)
		goto free_backend;

	/* setup the info needed to clean-up the backend later */
	new->dl = dl;
	new->fini = fini;

	return VACCEL_OK;

free_backend:
	free(new);

close_dl:
	dlclose(dl);
	return ret;
}

int load_backend_plugins(char *plugins)
{
	char *plugin = strtok(plugins, ";");
	while (plugin) {
		int ret = load_backend_plugin(plugin);
		if (ret != VACCEL_OK)
			return ret;

		plugin = strtok(NULL, ";");
	}

	return VACCEL_OK;
}

__attribute__((constructor))
static void vaccel_init(void)
{
	/* initialize the backends system */
	backends_bootstrap();

	/* find backend implementations and set them up */
	char *plugins = getenv("VACCEL_BACKENDS");
	if (!plugins)
		return;

	load_backend_plugins(plugins);
}

__attribute__((destructor))
static void vaccel_fini(void)
{
	cleanup_backends();
}
