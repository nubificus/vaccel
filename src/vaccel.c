#include "vaccel.h"
#include "backend.h"

#include <stdbool.h>

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


__attribute__((constructor))
static void vaccel_init(void)
{
	/* initialize the backends system */
	backends_bootstrap();

	/* find backend implementations and set them up */
}

__attribute__((destructor))
static void vaccel_fini(void)
{
}
