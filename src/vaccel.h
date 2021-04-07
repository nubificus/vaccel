#ifndef __VACCEL_H__
#define __VACCEL_H__

#include <stdint.h>
#include <stddef.h>

#include "common.h"

#include "ops/image.h"
#include "ops/noop.h"

#ifdef __cplusplus
extern "C" {
#endif

struct vaccel_session {
	/* id of the session */
	uint32_t session_id;

	/* backend private data */
	void *priv;
};

/* Initialize a new session with the runtime */
int vaccel_sess_init(struct vaccel_session *sess, uint32_t flags);

/* Tear down a session */
int vaccel_sess_free(struct vaccel_session *sess);

#ifdef __cplusplus
}
#endif

#endif /* __VACCEL_H__ */
