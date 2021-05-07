#include "blas.h"
#include "error.h"
#include "plugin.h"
#include "log.h"
#include "vaccel_ops.h"
#include "genop.h"

#include "session.h"

int vaccel_sgemm(struct vaccel_session *sess, uint32_t k, uint32_t m,
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

int vaccel_sgemm_unpack(struct vaccel_session *sess, struct vaccel_arg *read,
		int nr_read, struct vaccel_arg *write, int nr_write)
{
	if (nr_read != 5) {
		vaccel_error("Wrong number of read arguments in SGEMM: %d",
				nr_read);
		return VACCEL_EINVAL;
	}

	if (nr_write != 1) {
		vaccel_error("Wrong number of write arguments in SGEMM: %d",
				nr_write);
		return VACCEL_EINVAL;
	}

	uint32_t k = *(uint32_t*)read[0].buf;
	uint32_t m = *(uint32_t*)read[1].buf;
	uint32_t n = *(uint32_t*)read[2].buf;
	size_t len_a = (size_t)read[3].size;
	float *a = (float *)read[3].buf;
	size_t len_b = (size_t)read[4].size;
	float *b = (float *)read[4].buf;

	size_t len_c = (size_t)write[0].size;
	float *c = (float *)write[0].buf;

	return vaccel_sgemm(sess, k, m, n, len_a, len_b, len_c, a, b, c);
}
