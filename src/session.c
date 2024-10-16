// SPDX-License-Identifier: Apache-2.0

#include "session.h"
#include "id_pool.h"
#include "log.h"
#include "plugin.h"
#include "utils.h"

#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

enum { MAX_VACCEL_SESSIONS = 1024 };

struct {
	/* true if the sessions subsystem has been initialized */
	bool initialized;

	/* Available session ids */
	id_pool_t ids;

	/* Active sessions */
	struct vaccel_session *running_sessions[MAX_VACCEL_SESSIONS];
} sessions;

static uint32_t get_sess_id(void)
{
	return id_pool_get(&sessions.ids);
}

static void put_sess_id(uint32_t id)
{
	id_pool_release(&sessions.ids, id);
}

int sessions_bootstrap(void)
{
	int ret = id_pool_new(&sessions.ids, MAX_VACCEL_SESSIONS);
	if (ret)
		return ret;

	for (size_t i = 0; i < MAX_VACCEL_SESSIONS; ++i)
		sessions.running_sessions[i] = NULL;

	sessions.initialized = true;

	return VACCEL_OK;
}

int sessions_cleanup(void)
{
	if (!sessions.initialized)
		return VACCEL_OK;

	id_pool_destroy(&sessions.ids);
	sessions.initialized = false;

	return VACCEL_OK;
}

int vaccel_session_register_resource(struct vaccel_session *sess,
				     struct vaccel_resource *res)
{
	int ret;

	if (!sess || !sess->resources || !res ||
	    res->type >= VACCEL_RESOURCE_MAX)
		return VACCEL_EINVAL;

	struct session_resources *resources = sess->resources;
	struct registered_resource *container;

	if (sess->is_virtio) {
		struct vaccel_plugin *virtio = get_virtio_plugin();
		if (virtio) {
			if (res->remote_id <= 0) {
				ret = virtio->info->resource_new(
					res->type, NULL, &res->remote_id);
				if (res->remote_id <= 0 || ret) {
					vaccel_error(
						"Could not create remote resource");
					return ret;
				}
			}
			ret = virtio->info->sess_register(sess->remote_id,
							  res->remote_id);
			if (ret) {
				vaccel_error(
					"Could not register remote resource");
				return ret;
			}
		} else {
			vaccel_error(
				"Could not register resource to virtio session, no VirtIO Plugin loaded yet");
			return VACCEL_ENOTSUP;
		}
	}

	container = malloc(sizeof(*container));
	if (!container)
		return VACCEL_ENOMEM;

	container->res = res;
	list_add_tail(&resources->registered[res->type], &container->entry);
	resource_refcount_inc(res);

	vaccel_debug("Registered resource %lld to session %" PRIu32, res->id,
		     sess->session_id);

	return VACCEL_OK;
}

static struct registered_resource *
find_registered_resource(const struct vaccel_session *sess,
			 const struct vaccel_resource *res)
{
	struct session_resources *resources = sess->resources;
	list_t *list = &resources->registered[res->type];

	struct registered_resource *iter = NULL;
	for_each_session_resource(iter, list) {
		if (iter->res == res)
			return iter;
	}

	return NULL;
}

int vaccel_session_unregister_resource(struct vaccel_session *sess,
				       struct vaccel_resource *res)
{
	int ret;

	if (!sess || !res || res->type >= VACCEL_RESOURCE_MAX)
		return VACCEL_EINVAL;

	/* Check if resource is indeed registered to session */
	struct registered_resource *container =
		find_registered_resource(sess, res);

	if (!container) {
		vaccel_error("Resource %u not registered with session %" PRIu32,
			     res->id, sess->session_id);
		return VACCEL_EINVAL;
	}

	if (sess->is_virtio) {
		struct vaccel_plugin *virtio = get_virtio_plugin();
		if (virtio) {
			ret = virtio->info->sess_unregister(sess->remote_id,
							    res->remote_id);
			if (ret) {
				vaccel_error(
					"BUG: Could not unregister host-side resource %" PRIu32,
					res->remote_id);
				return ret;
			}

			ret = virtio->info->resource_destroy(res->remote_id);
			if (ret) {
				vaccel_warn(
					"Could not destroy host-side resource %" PRIu32,
					res->remote_id);
			}
		} else {
			vaccel_error(
				"Could not unregister resource for virtio session, no VirtIO Plugin loaded yet");
			return VACCEL_ENOTSUP;
		}
	}

	list_unlink_entry(&container->entry);
	resource_refcount_dec(container->res);
	free(container);

	vaccel_debug("Unregistered resource %lld from session %" PRIu32,
		     res->id, sess->session_id);

	return VACCEL_OK;
}

bool vaccel_session_has_resource(const struct vaccel_session *sess,
				 const struct vaccel_resource *res)
{
	return find_registered_resource(sess, res) != NULL;
}

static int session_initialize_resources(struct vaccel_session *sess)
{
	if (!sess)
		return VACCEL_EINVAL;

	struct session_resources *res = malloc(sizeof(*res));
	if (!res)
		return VACCEL_ENOMEM;

	const char *root_rundir = vaccel_rundir();
	int ret = snprintf(res->rundir, MAX_SESSION_RUNDIR_PATH,
			   "%s/session.%" PRIu32, root_rundir,
			   sess->session_id);
	if (ret == MAX_SESSION_RUNDIR_PATH) {
		vaccel_error("rundir path '%s/session.%" PRIu32 "' too big",
			     root_rundir, sess->session_id);
		ret = VACCEL_ENAMETOOLONG;
		goto cleanup_res;
	}

	ret = mkdir(res->rundir, 0700);
	if (ret)
		goto cleanup_res;

	sess->resources = res;
	for (int i = 0; i < VACCEL_RESOURCE_MAX; ++i)
		list_init(&res->registered[i]);

	return VACCEL_OK;

cleanup_res:
	free(res);
	return ret;
}

static int session_cleanup_resources(struct vaccel_session *sess)
{
	if (!sess || !sess->resources)
		return VACCEL_EINVAL;

	struct session_resources *resources = sess->resources;
	for (int i = 0; i < VACCEL_RESOURCE_MAX; ++i) {
		struct registered_resource *iter = NULL;
		struct registered_resource *tmp;
		for_each_session_resource_safe(iter, tmp,
					       &resources->registered[i]) {
			int ret = vaccel_session_unregister_resource(sess,
								     iter->res);
			if (ret) {
				vaccel_error(
					"Could not unregister resource from session");
				return ret;
			}
		}
	}

	/* Try to cleanup the rundir. At the moment, we do not fail
	 * if this fails, we just warn the user */
	int ret = cleanup_rundir(sess->resources->rundir);
	if (ret)
		vaccel_warn(
			"Could not cleanup rundir '%s' for session %" PRIu32,
			sess->resources->rundir, sess->session_id);

	free(sess->resources);
	sess->resources = NULL;

	return VACCEL_OK;
}

int vaccel_session_init(struct vaccel_session *sess, uint32_t flags)
{
	int ret;
	struct vaccel_plugin *virtio = NULL;

	if (!sess)
		return VACCEL_EINVAL;

	if (!sessions.initialized)
		return VACCEL_ESESS;

	virtio = get_virtio_plugin();
	if ((flags & VACCEL_REMOTE) || (get_nr_plugins() == 1 && virtio)) {
		if (!virtio) {
			vaccel_error(
				"Could not initialize VirtIO session, no VirtIO Plugin loaded yet");
			return VACCEL_ENOTSUP;
		}

		ret = virtio->info->sess_init(sess, flags & (~VACCEL_REMOTE));
		if (ret) {
			vaccel_error("Could not create host-side session");
			return ret;
		}

		sess->is_virtio = true;
	} else {
		sess->is_virtio = false;
	}

	uint32_t sess_id = get_sess_id();

	if (!sess_id)
		return VACCEL_ESESS;

	sess->session_id = sess_id;

	ret = session_initialize_resources(sess);
	if (ret)
		goto cleanup_session;

	sess->hint = flags;

	vaccel_debug("session:%" PRIu32 " New session", sess->session_id);

	sessions.running_sessions[sess->session_id - 1] = sess;

	return VACCEL_OK;

cleanup_session:
	if (sess->is_virtio) {
		if (virtio->info->sess_free(sess)) {
			vaccel_error(
				"BUG: Could not cleanup host-side session");
		}
	}

	put_sess_id(sess->session_id);

	return ret;
}

int vaccel_session_update(struct vaccel_session *sess, uint32_t flags)
{
	if (!sess)
		return VACCEL_EINVAL;

	if (!sessions.initialized)
		return VACCEL_ESESS;

	/* if we're using virtio as a plugin offload the session update to the
	 * host */
	if (sess->is_virtio) {
		struct vaccel_plugin *virtio = get_virtio_plugin();
		if (virtio) {
			int ret = virtio->info->sess_update(
				sess, flags & (~VACCEL_REMOTE));
			if (ret) {
				vaccel_error(
					"Could not update host-side session");
				return ret;
			}
		} else {
			vaccel_error(
				"Could not update virtio session, no VirtIO Plugin loaded yet");
			return VACCEL_ENOTSUP;
		}
	} else {
		sess->hint = flags;
	}

	vaccel_debug("session:%" PRIu32 " update with flags: %u",
		     sess->session_id, flags);

	return VACCEL_OK;
}

int vaccel_session_free(struct vaccel_session *sess)
{
	int ret;

	if (!sess)
		return VACCEL_EINVAL;

	if (!sessions.initialized)
		return VACCEL_ESESS;

	/* if we're using virtio as a plugin offload the session cleanup to the
	 * host */
	if (sess->is_virtio) {
		struct vaccel_plugin *virtio = get_virtio_plugin();
		if (virtio) {
			ret = virtio->info->sess_free(sess);
			if (ret) {
				vaccel_warn(
					"Could not cleanup host-side session");
			}
		} else {
			vaccel_error(
				"Could not free VirtIO session, no VirtIO Plugin loaded yet");
			return VACCEL_ENOTSUP;
		}
	}

	ret = session_cleanup_resources(sess);
	if (ret) {
		vaccel_error("Could not cleanup session resources");
		return ret;
	}

	put_sess_id(sess->session_id);

	sessions.running_sessions[sess->session_id - 1] = NULL;

	vaccel_debug("session:%" PRIu32 " Free session", sess->session_id);

	return VACCEL_OK;
}

int vaccel_sess_init(struct vaccel_session *sess, uint32_t flags)
{
	vaccel_warn("%s%s", "vaccel_sess_init() is deprecated. ",
		    "Please use vaccel_session_init() instead.");
	return vaccel_session_init(sess, flags);
}

int vaccel_sess_update(struct vaccel_session *sess, uint32_t flags)
{
	vaccel_warn("%s%s", "vaccel_sess_update() is deprecated. ",
		    "Please use vaccel_session_update() instead.");
	return vaccel_session_update(sess, flags);
}

int vaccel_sess_free(struct vaccel_session *sess)
{
	vaccel_warn("%s%s", "vaccel_sess_free() is deprecated. ",
		    "Please use vaccel_session_free() instead.");
	return vaccel_session_free(sess);
}

bool vaccel_sess_has_resource(struct vaccel_session *sess,
			      struct vaccel_resource *res)
{
	vaccel_warn("%s%s", "vaccel_sess_has_resource() is deprecated. ",
		    "Please use vaccel_session_has_resource() instead.");
	return vaccel_session_has_resource(sess, res);
}
