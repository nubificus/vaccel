#ifndef __VACCEL_H__
#define __VACCEL_H__

#include <stdint.h>
#include <stddef.h>

#include "common.h"
#include "vaccel_ops.h"
#include "list.h"

#ifdef __cplusplus
extern "C" {
#endif

struct vaccel_session {
	/* id of the session */
	uint32_t session_id;

	/* List of resources registered with the
	 * session. This will do for now, but potentially
	 * we need a more clever data structure here */
	list_t resources;
};


/* Initialize a new session with the runtime */
int vaccel_sess_init(struct vaccel_session *sess, uint32_t flags);

/* Tear down a session */
int vaccel_sess_free(struct vaccel_session *sess);

#ifdef __cplusplus
}
#endif

#endif /* __VACCEL_H__ */
