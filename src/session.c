// SPDX-License-Identifier: Apache-2.0

#include "session.h"
#include "core.h"
#include "error.h"
#include "id_pool.h"
#include "list.h"
#include "log.h"
#include "plugin.h"
#include "resource.h"
#include "utils/fs.h"
#include "utils/path.h"
#include <inttypes.h>
#include <limits.h>
#include <linux/limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { VACCEL_SESSIONS_MAX = 1024 };

static struct {
	/* true if the sessions component has been initialized */
	bool initialized;

	/* available session ids */
	id_pool_t ids;

	/* all the created vAccel sessions */
	struct vaccel_session *all[VACCEL_SESSIONS_MAX];
} sessions;

static void get_session_id(struct vaccel_session *sess)
{
	sess->id = id_pool_get(&sessions.ids);
}

static void put_session_id(struct vaccel_session *sess)
{
	if (id_pool_put(&sessions.ids, sess->id))
		vaccel_warn("Could not return resource ID to pool");
	sess->id = 0;
}

int sessions_bootstrap(void)
{
	int ret = id_pool_init(&sessions.ids, VACCEL_SESSIONS_MAX);
	if (ret)
		return ret;

	for (size_t i = 0; i < VACCEL_SESSIONS_MAX; ++i)
		sessions.all[i] = NULL;

	sessions.initialized = true;

	return VACCEL_OK;
}

int sessions_cleanup(void)
{
	if (!sessions.initialized)
		return VACCEL_OK;

	vaccel_debug("Cleaning up sessions");

	for (int i = 0; i < VACCEL_SESSIONS_MAX; ++i)
		vaccel_session_release(sessions.all[i]);

	sessions.initialized = false;

	return id_pool_release(&sessions.ids);
}

static struct registered_resource *
find_registered_resource(const struct vaccel_session *sess,
			 const struct vaccel_resource *res)
{
	if (!sess || !res)
		return NULL;

	struct session_resources *resources = sess->resources;
	vaccel_list_t *list = &resources->registered[res->type];

	struct registered_resource *iter = NULL;
	session_for_each_resource(iter, list)
	{
		if (iter->res == res)
			return iter;
	}

	return NULL;
}

int session_register_resource(struct vaccel_session *sess,
			      struct vaccel_resource *res)
{
	if (!sess || !sess->resources || !res ||
	    res->type >= VACCEL_RESOURCE_MAX)
		return VACCEL_EINVAL;

	/* Check if resource is already registered with session */
	struct registered_resource *container =
		find_registered_resource(sess, res);
	if (container) {
		vaccel_error("session:%" PRId64 " Resource %" PRId64
			     " already registered",
			     sess->id, res->id);
		return VACCEL_EINVAL;
	}

	struct session_resources *resources = sess->resources;
	container = malloc(sizeof(*container));
	if (!container)
		return VACCEL_ENOMEM;

	container->res = res;
	list_add_tail(&resources->registered[res->type], &container->entry);

	return VACCEL_OK;
}

int session_unregister_resource(struct vaccel_session *sess,
				struct vaccel_resource *res)
{
	if (!sess || !res || res->type >= VACCEL_RESOURCE_MAX)
		return VACCEL_EINVAL;

	/* Check if resource is indeed registered with session */
	struct registered_resource *container =
		find_registered_resource(sess, res);
	if (!container) {
		vaccel_error("session:%" PRId64 " Resource %" PRId64
			     " not registered",
			     sess->id, res->id);
		return VACCEL_EINVAL;
	}

	list_unlink_entry(&container->entry);
	free(container);

	return VACCEL_OK;
}

static int session_initialize_resources(struct vaccel_session *sess)
{
	if (!sess || sess->id <= 0) {
		vaccel_error("Trying to create rundir for invalid session");
		return VACCEL_EINVAL;
	}

	struct session_resources *res = malloc(sizeof(*res));
	if (!res)
		return VACCEL_ENOMEM;

	char sess_dir[NAME_MAX];
	int ret = snprintf(sess_dir, NAME_MAX, "session.%" PRId64, sess->id);
	if (ret < 0) {
		vaccel_error("Could not generate session %" PRId64
			     " rundir name",
			     sess->id);
		goto free;
	}
	if (ret == NAME_MAX) {
		vaccel_error("Session %" PRId64 " rundir name too long",
			     sess->id);
		ret = VACCEL_ENAMETOOLONG;
		goto free;
	}

	ret = path_init_from_parts(res->rundir, PATH_MAX, vaccel_rundir(),
				   sess_dir, NULL);
	if (ret) {
		vaccel_error(
			"Could not generate rundir path for session %" PRId64,
			sess->id);
		goto free;
	}

	ret = fs_dir_create(res->rundir);
	if (ret) {
		vaccel_error("Could not create rundir for session %" PRId64,
			     sess->id);
		goto free;
	}

	vaccel_debug("New rundir for session %" PRId64 ": %s", sess->id,
		     res->rundir);

	sess->resources = res;
	for (int i = 0; i < VACCEL_RESOURCE_MAX; ++i)
		list_init(&res->registered[i]);

	return VACCEL_OK;

free:
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
		session_for_each_resource_safe(iter, tmp,
					       &resources->registered[i])
		{
			int ret = vaccel_resource_unregister(iter->res, sess);
			if (ret) {
				vaccel_error(
					"Could not unregister resource from session");
				return ret;
			}
		}
	}

	/* Try to cleanup the rundir. At the moment, we do not fail
	 * if this fails, we just warn the user */
	int ret = fs_dir_remove(sess->resources->rundir);
	if (ret)
		vaccel_warn(
			"Could not cleanup rundir '%s' for session %" PRId64,
			sess->resources->rundir, sess->id);

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

	get_session_id(sess);
	if (sess->id < 0)
		return -(int)sess->id;

	sess->priv = NULL;

	virtio = plugin_virtio();
	if ((flags & VACCEL_PLUGIN_REMOTE) ||
	    (plugin_nr_registered() == 1 && virtio)) {
		if (!virtio) {
			vaccel_error(
				"Could not initialize VirtIO session, no VirtIO Plugin loaded yet");
			ret = VACCEL_ENOTSUP;
			goto release_id;
		}

		ret = virtio->info->session_init(
			sess, flags & (~VACCEL_PLUGIN_REMOTE));
		if (ret) {
			vaccel_error("Could not create host-side session");
			goto release_id;
		}

		sess->is_virtio = true;
	} else {
		sess->is_virtio = false;
	}

	ret = session_initialize_resources(sess);
	if (ret)
		goto cleanup_session;

	sess->hint = flags;
	sessions.all[sess->id - 1] = sess;

	if (sess->is_virtio)
		vaccel_debug("Initialized session %" PRId64
			     " with remote (id: %" PRId64 ")",
			     sess->id, sess->remote_id);
	else
		vaccel_debug("Initialized session %" PRId64, sess->id);

	return VACCEL_OK;

cleanup_session:
	if (sess->is_virtio) {
		if (virtio->info->session_release(sess)) {
			vaccel_error(
				"BUG: Could not cleanup host-side session");
		}
	}
release_id:
	put_session_id(sess);

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
		struct vaccel_plugin *virtio = plugin_virtio();
		if (virtio) {
			int ret = virtio->info->session_update(
				sess, flags & (~VACCEL_PLUGIN_REMOTE));
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

	vaccel_debug("session:%" PRId64 " Updated with flags: %u", sess->id,
		     flags);

	return VACCEL_OK;
}

int vaccel_session_release(struct vaccel_session *sess)
{
	int ret;

	if (!sess)
		return VACCEL_EINVAL;

	if (!sessions.initialized)
		return VACCEL_ESESS;

	if (sess->id <= 0) {
		vaccel_error("Cannot release uninitialized session");
		return VACCEL_EINVAL;
	}

	ret = session_cleanup_resources(sess);
	if (ret) {
		vaccel_error("Could not cleanup session resources");
		return ret;
	}

	/* If we're using virtio as a plugin, offload the session cleanup to the
	 * host */
	if (sess->is_virtio) {
		struct vaccel_plugin *virtio = plugin_virtio();
		if (virtio) {
			ret = virtio->info->session_release(sess);
			if (ret) {
				vaccel_warn(
					"Could not release host-side session");
			}
		} else {
			vaccel_error(
				"Could not release VirtIO session, no VirtIO Plugin loaded yet");
			return VACCEL_ENOTSUP;
		}
	}

	sess->priv = NULL;
	sessions.all[sess->id - 1] = NULL;

	vaccel_debug("Released session %" PRId64, sess->id);

	put_session_id(sess);

	return VACCEL_OK;
}

int vaccel_session_new(struct vaccel_session **sess, uint32_t flags)
{
	if (!sess)
		return VACCEL_EINVAL;

	struct vaccel_session *s =
		(struct vaccel_session *)malloc(sizeof(struct vaccel_session));
	if (!s)
		return VACCEL_ENOMEM;

	int ret = vaccel_session_init(s, flags);
	if (ret) {
		free(s);
		return ret;
	}

	*sess = s;

	return VACCEL_OK;
}

int vaccel_session_delete(struct vaccel_session *sess)
{
	int ret = vaccel_session_release(sess);
	if (ret)
		return ret;

	free(sess);

	return VACCEL_OK;
}

bool vaccel_session_has_resource(const struct vaccel_session *sess,
				 const struct vaccel_resource *res)
{
	return find_registered_resource(sess, res) != NULL;
}

int vaccel_session_resource_by_id(struct vaccel_session *sess,
				  struct vaccel_resource **res, vaccel_id_t id)
{
	if (!sess || !res || id <= 0)
		return VACCEL_EINVAL;

	struct session_resources *resources = sess->resources;

	for (int type = 0; type < VACCEL_RESOURCE_MAX; ++type) {
		vaccel_list_t *list = &resources->registered[type];
		struct registered_resource *iter = NULL;

		session_for_each_resource(iter, list)
		{
			if (iter->res->id == id) {
				*res = iter->res;
				return VACCEL_OK;
			}
		}
	}

	return VACCEL_ENOENT;
}

int vaccel_session_resource_by_type(struct vaccel_session *sess,
				    struct vaccel_resource **res,
				    vaccel_resource_type_t type)
{
	if (!sess || !res || type >= VACCEL_RESOURCE_MAX) {
		return VACCEL_EINVAL;
	}

	struct session_resources *resources = sess->resources;
	vaccel_list_t *list = &resources->registered[type];

	struct registered_resource *iter = NULL;
	session_for_each_resource(iter, list)
	{
		*res = iter->res;
		return VACCEL_OK;
	}

	return VACCEL_ENOENT;
}

int vaccel_session_resources_by_type(struct vaccel_session *sess,
				     struct vaccel_resource ***resources,
				     size_t *nr_found,
				     vaccel_resource_type_t type)
{
	if (!sess || !resources || type >= VACCEL_RESOURCE_MAX || !nr_found)
		return VACCEL_EINVAL;

	struct session_resources *sess_resources = sess->resources;
	vaccel_list_t *list = &sess_resources->registered[type];

	size_t found = 0;
	struct registered_resource *iter = NULL;

	/* Count matching resources */
	session_for_each_resource(iter, list)
	{
		++found;
	}

	if (found == 0) {
		*nr_found = 0;
		return VACCEL_ENOENT;
	}

	/* Allocate space */
	*resources = (struct vaccel_resource **)malloc(
		found * sizeof(struct vaccel_resource *));
	if (*resources == NULL)
		return VACCEL_ENOMEM;

	iter = NULL;
	size_t i = 0;
	session_for_each_resource(iter, list)
	{
		(*resources)[i++] = iter->res;
	}

	*nr_found = found;
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
		    "Please use vaccel_session_release() instead.");
	return vaccel_session_release(sess);
}

bool vaccel_sess_has_resource(struct vaccel_session *sess,
			      struct vaccel_resource *res)
{
	vaccel_warn("%s%s", "vaccel_sess_has_resource() is deprecated. ",
		    "Please use vaccel_session_has_resource() instead.");
	return vaccel_session_has_resource(sess, res);
}
