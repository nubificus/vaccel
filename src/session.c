#include "vaccel.h"

#include <stdint.h>

int vaccel_sess_init(struct vaccel_session *sess, uint32_t flags)
{
	(void)sess;
	(void)flags;

	return VACCEL_OK;
}

int vaccel_sess_free(struct vaccel_session *sess)
{
	(void)sess;

	return VACCEL_OK;
}
