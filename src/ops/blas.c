/*
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "blas.h"
#include "error.h"
#include "plugin.h"
#include "log.h"
#include "vaccel_ops.h"
#include "genop.h"

#include "session.h"

int vaccel_sgemm(
	struct vaccel_session *sess,
	long long int m, long long int n, long long int k,
	float alpha,
	float *a, long long int lda,
	float *b, long long int ldb,
	float beta,
	float *c, long long int ldc
) {
	if (!sess)
		return VACCEL_EINVAL;

	vaccel_debug("session:%u Looking for plugin implementing BLAS SGEMM",
			sess->session_id);

	//Get implementation
	int (*plugin_op)(
		struct vaccel_session *sess,
		long long int m, long long int n, long long int k,
		float alpha,
		float *a, long long int lda,
		float *b, long long int ldb,
		float beta,
		float *c, long long int ldc
	) = get_plugin_op(VACCEL_BLAS_SGEMM, sess->hint);
	if (!plugin_op)
		return VACCEL_ENOTSUP;

	return plugin_op(sess, m, n, k, alpha, a, lda, b, ldb, beta, c, ldc);
}

int vaccel_sgemm_unpack(
	struct vaccel_session *sess,
	struct vaccel_arg *read, int nr_read,
	struct vaccel_arg *write, int nr_write
) {
	if (nr_read != 7) {
		vaccel_error("Wrong number of read arguments in SGEMM: %d",
				nr_read);
		return VACCEL_EINVAL;
	}

	if (nr_write != 1) {
		vaccel_error("Wrong number of write arguments in SGEMM: %d",
				nr_write);
		return VACCEL_EINVAL;
	}

	long long int m = *(long long int*)read[0].buf;
	long long int n = *(long long int*)read[1].buf;
	long long int k = *(long long int*)read[2].buf;
	float alpha = *(float *)read[3].buf;
	long long int lda  = (long long int)read[4].size;
	float *a = (float *)read[4].buf;
	long long int ldb = (long long int)read[5].size;
	float *b = (float *)read[5].buf;
	float beta = *(float *)read[6].buf;

	long long int ldc = (long long int)write[0].size;
	float *c = (float *)write[0].buf;

	return vaccel_sgemm(sess, m, n, k, alpha, a, lda, b, ldb, beta, c, ldc);
}
