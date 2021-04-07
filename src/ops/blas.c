#include "blas.h"
#include "common.h"
#include "plugin.h"
#include "log.h"
#include "vaccel_ops.h"

int virtio_sgemm(struct vaccel_session *sess, uint32_t k, uint32_t m,
		uint32_t n, size_t len_a, size_t len_b, size_t len_c,
		float *a, float *b, float *c)
{
	if (!sess)
		return VACCEL_EINVAL;

	vaccel_debug("session:%u Looking for plugin implementing BLAS SGEMM",
			sess->session_id);

	//Get implementation
	int (*plugin_op)() = get_plugin_op(VACCEL_BLAS_SGEMM);
	if (!plugin_op)
		return VACCEL_ENOTSUP;

	return plugin_op(sess, k, m, n, len_a, len_b, len_c, a, b, c);
}
