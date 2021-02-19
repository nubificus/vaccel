#ifndef __VACCEL_SESSION_H__
#define __VACCEL_SESSION_H__

#include "list.h"
#include "vaccel.h"

#include <stdint.h>
#include <stdbool.h>

struct vaccel_resource;

int sessions_bootstrap(void);

int session_register_resource(struct vaccel_session *session,
		struct vaccel_resource *resource);
int session_unregister_resource(struct vaccel_session *session,
		struct vaccel_resource *resource);
bool session_has_resource(struct vaccel_session *session,
		struct vaccel_resource *resource);

#endif /* __VACCEL_SESSION_H__ */
