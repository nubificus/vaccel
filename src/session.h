#ifndef __VACCEL_SESSION_H__
#define __VACCEL_SESSION_H__

#include "list.h"

#include <stdint.h>

struct vaccel_resource;

struct vaccel_session {
	/* id of the session */
	uint32_t session_id;

	/* List of resources registered with the
	 * session. This will do for now, but potentially
	 * we need a more clever data structure here */
	list_t resources;
};

int sessions_bootstrap(void);

int session_register_resource(struct vaccel_session *session, struct vaccel_resource *resource);
int session_unregister_resource(struct vaccel_session *session, struct vaccel_resource *resource);
int session_has_resource(struct vaccel_session *session, struct vaccel_resource *resource);

#endif /* __VACCEL_SESSION_H__ */
