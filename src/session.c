// SPDX-License-Identifier: Apache-2.0

#include "session.h"
#include "core.h"
#include "error.h"
#include "id_pool.h"
#include "list.h"
#include "log.h"
#include "plugin.h"
#include "resource.h"
#include "resource_registration.h"
#include "utils/fs.h"
#include "utils/path.h"
#include <inttypes.h>
#include <limits.h>
#include <linux/limits.h>
#include <pthread.h>
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

	/* list of all the created sessions */
	struct vaccel_list_entry all;

	/* counter for all created sessions */
	size_t count;

	/* lock for list/counter */
	pthread_mutex_t lock;
} sessions = { .initialized = false };

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

	list_init(&sessions.all);
	sessions.count = 0;
	pthread_mutex_init(&sessions.lock, NULL);

	sessions.initialized = true;
	return VACCEL_OK;
}

int sessions_cleanup(void)
{
	if (!sessions.initialized)
		return VACCEL_OK;

	vaccel_debug("Cleaning up sessions");

	pthread_mutex_lock(&sessions.lock);

	struct vaccel_session *sess;
	struct vaccel_session *tmp;
	session_for_each_safe(sess, tmp, &sessions.all)
	{
		pthread_mutex_unlock(&sessions.lock);
		vaccel_session_release(sess);
		pthread_mutex_lock(&sessions.lock);
	}

	pthread_mutex_unlock(&sessions.lock);

	pthread_mutex_destroy(&sessions.lock);
	sessions.initialized = false;

	return id_pool_release(&sessions.ids);
}

int vaccel_session_get_by_id(struct vaccel_session **sess, vaccel_id_t id)
{
	if (!sessions.initialized)
		return VACCEL_EPERM;

	if (!sess)
		return VACCEL_EINVAL;

	pthread_mutex_lock(&sessions.lock);

	struct vaccel_session *s;
	session_for_each(s, &sessions.all)
	{
		if (id == s->id) {
			*sess = s;
			pthread_mutex_unlock(&sessions.lock);
			return VACCEL_OK;
		}
	}

	pthread_mutex_unlock(&sessions.lock);
	return VACCEL_ENOENT;
}

static int session_create_rundir(struct vaccel_session *sess)
{
	if (!sess || sess->id <= 0) {
		vaccel_error("Trying to create rundir for invalid session");
		return VACCEL_EINVAL;
	}

	char sess_dir[NAME_MAX];
	int ret = snprintf(sess_dir, NAME_MAX, "session.%" PRId64, sess->id);
	if (ret < 0) {
		vaccel_error("Could not generate session %" PRId64
			     " rundir name",
			     sess->id);
		return ret;
	}
	if (ret == NAME_MAX) {
		vaccel_error("Session %" PRId64 " rundir name too long",
			     sess->id);
		return VACCEL_ENAMETOOLONG;
	}

	ret = path_init_from_parts(sess->rundir, PATH_MAX, vaccel_rundir(),
				   sess_dir, NULL);
	if (ret) {
		vaccel_error(
			"Could not generate rundir path for session %" PRId64,
			sess->id);
		return ret;
	}

	ret = fs_dir_create(sess->rundir);
	if (ret) {
		vaccel_error("Could not create rundir for session %" PRId64,
			     sess->id);
		return ret;
	}

	vaccel_debug("New rundir for session %" PRId64 ": %s", sess->id,
		     sess->rundir);

	return VACCEL_OK;
}

static int session_destroy_rundir(struct vaccel_session *sess)
{
	if (!sess)
		return VACCEL_EINVAL;

	if (fs_dir_remove(sess->rundir))
		vaccel_warn(
			"Could not cleanup rundir '%s' for session %" PRId64,
			sess->rundir, sess->id);

	return VACCEL_OK;
}

int vaccel_session_init(struct vaccel_session *sess, uint32_t flags)
{
	int ret;

	if (!sess)
		return VACCEL_EINVAL;

	if (!sessions.initialized)
		return VACCEL_ESESS;

	get_session_id(sess);
	if (sess->id < 0)
		return -(int)sess->id;

	sess->plugin = plugin_find(flags);
	if (!sess->plugin) {
		ret = VACCEL_ENOTSUP;
		goto release_id;
	}

	sess->priv = NULL;

	if ((flags & VACCEL_PLUGIN_REMOTE) ||
	    (plugin_count() == 1 && sess->plugin->info->is_virtio)) {
		if (!sess->plugin->info->is_virtio) {
			vaccel_error(
				"Cannot initialize remote session; no VirtIO plugin loaded yet");
			ret = VACCEL_ENOTSUP;
			goto release_id;
		}
		if (!sess->plugin->info->session_init) {
			vaccel_error(
				"Cannot initialize remote session; invalid plugin `session_init()`");
			ret = VACCEL_ENOTSUP;
			goto release_id;
		}

		ret = sess->plugin->info->session_init(
			sess, flags & (~VACCEL_PLUGIN_REMOTE));
		if (ret) {
			vaccel_error("Failed to initialize remote session");
			goto release_id;
		}

		sess->is_virtio = true;
	} else {
		sess->is_virtio = false;
	}

	ret = session_create_rundir(sess);
	if (ret)
		goto cleanup_session;

	for (size_t i = 0; i < VACCEL_RESOURCE_MAX; i++) {
		list_init(&sess->resources[i]);
		sess->resource_counts[i] = 0;
	}
	pthread_mutex_init(&sess->resources_lock, NULL);

	sess->hint = flags;

	pthread_mutex_lock(&sessions.lock);
	list_add_tail(&sessions.all, &sess->entry);
	sessions.count++;
	pthread_mutex_unlock(&sessions.lock);

	if (sess->is_virtio)
		vaccel_debug("Initialized session %" PRId64
			     " with plugin %s (remote id: %" PRId64 ")",
			     sess->id, sess->plugin->info->name,
			     sess->remote_id);
	else
		vaccel_debug("Initialized session %" PRId64 " with plugin %s",
			     sess->id, sess->plugin->info->name);

	return VACCEL_OK;

cleanup_session:
	if (sess->is_virtio) {
		if (sess->plugin->info->session_release(sess))
			vaccel_error("Could not release remote session");
	}
release_id:
	sess->plugin = NULL;
	put_session_id(sess);

	return ret;
}

static bool session_has_resources(struct vaccel_session *sess)
{
	if (!sess)
		return false;

	pthread_mutex_lock(&sess->resources_lock);

	for (size_t type = 0; type < VACCEL_RESOURCE_MAX; type++) {
		if (sess->resource_counts[type] > 0) {
			pthread_mutex_unlock(&sess->resources_lock);
			return true;
		}
	}

	pthread_mutex_unlock(&sess->resources_lock);
	return false;
}

int vaccel_session_update(struct vaccel_session *sess, uint32_t flags)
{
	if (!sess)
		return VACCEL_EINVAL;

	if (!sessions.initialized)
		return VACCEL_ESESS;

	/* If the plugin is virtio, forward session updating to the host */
	if (sess->is_virtio) {
		if (!sess->plugin->info->is_virtio) {
			vaccel_error(
				"Cannot update remote session; no VirtIO plugin loaded yet");
			return VACCEL_ENOTSUP;
		}
		if (!sess->plugin->info->session_update) {
			vaccel_error(
				"Cannot update remote session; invalid plugin `session_update()`");
			return VACCEL_ENOTSUP;
		}

		int ret = sess->plugin->info->session_update(
			sess, flags & (~VACCEL_PLUGIN_REMOTE));
		if (ret)
			vaccel_error("Failed to update remote session");

		return ret;
	}

	if (session_has_resources(sess)) {
		vaccel_error(
			"Cannot update a session with registered resources");
		return VACCEL_ENOTSUP;
	}

	sess->plugin = plugin_find(flags);
	if (!sess->plugin)
		return VACCEL_ENOTSUP;

	sess->hint = flags;
	vaccel_debug("session:%" PRId64 " Selected plugin %s", sess->id,
		     sess->plugin->info->name);

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

	ret = resource_registration_foreach_resource(
		sess, vaccel_resource_unregister);
	if (ret) {
		vaccel_error("Could not unregister session resources");
		return ret;
	}

	pthread_mutex_destroy(&sess->resources_lock);

	ret = session_destroy_rundir(sess);
	if (ret)
		return ret;

	if (sess->is_virtio) {
		if (!sess->plugin->info->is_virtio) {
			vaccel_error(
				"Cannot release remote session; no VirtIO plugin loaded yet");
			return VACCEL_ENOTSUP;
		}
		if (!sess->plugin->info->session_release) {
			vaccel_error(
				"Cannot release remote session; invalid plugin `session_release()`");
			return VACCEL_ENOTSUP;
		}

		ret = sess->plugin->info->session_release(sess);
		if (ret)
			vaccel_warn("Could not release remote session");
	}

	sess->priv = NULL;
	sess->plugin = NULL;

	pthread_mutex_lock(&sessions.lock);
	list_unlink_entry(&sess->entry);
	sessions.count--;
	pthread_mutex_unlock(&sessions.lock);

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

bool vaccel_session_has_resource(struct vaccel_session *sess,
				 struct vaccel_resource *res)
{
	return resource_registration_find(res, sess) != NULL;
}

int vaccel_session_resource_by_id(struct vaccel_session *sess,
				  struct vaccel_resource **res, vaccel_id_t id)
{
	if (!sess || !res || id <= 0)
		return VACCEL_EINVAL;

	pthread_mutex_lock(&sess->resources_lock);

	for (size_t type = 0; type < VACCEL_RESOURCE_MAX; type++) {
		struct resource_registration *reg;
		session_for_each_resource(reg, &sess->resources[type])
		{
			if (reg->resource && reg->resource->id == id) {
				*res = reg->resource;
				pthread_mutex_unlock(&sess->resources_lock);
				return VACCEL_OK;
			}
		}
	}

	pthread_mutex_unlock(&sess->resources_lock);
	return VACCEL_ENOENT;
}

int vaccel_session_resource_by_type(struct vaccel_session *sess,
				    struct vaccel_resource **res,
				    vaccel_resource_type_t type)
{
	if (!sess || !res || type >= VACCEL_RESOURCE_MAX)
		return VACCEL_EINVAL;

	pthread_mutex_lock(&sess->resources_lock);

	if (list_empty(&sess->resources[type])) {
		pthread_mutex_unlock(&sess->resources_lock);
		return VACCEL_ENOENT;
	}

	struct resource_registration *reg =
		list_get_container(sess->resources[type].next,
				   struct resource_registration, session_entry);
	*res = reg->resource;

	pthread_mutex_unlock(&sess->resources_lock);
	return VACCEL_OK;
}

int vaccel_session_resources_by_type(struct vaccel_session *sess,
				     struct vaccel_resource ***resources,
				     size_t *nr_found,
				     vaccel_resource_type_t type)
{
	if (!sess || !resources || type >= VACCEL_RESOURCE_MAX || !nr_found)
		return VACCEL_EINVAL;

	pthread_mutex_lock(&sess->resources_lock);
	size_t nr_res = sess->resource_counts[type];
	pthread_mutex_unlock(&sess->resources_lock);

	if (nr_res == 0) {
		*nr_found = 0;
		return VACCEL_ENOENT;
	}

	*resources = (struct vaccel_resource **)malloc(
		nr_res * sizeof(struct vaccel_resource *));
	if (!*resources)
		return VACCEL_ENOMEM;

	pthread_mutex_lock(&sess->resources_lock);

	size_t cnt = 0;
	struct resource_registration *reg;
	session_for_each_resource(reg, &sess->resources[type])
	{
		if (cnt < nr_res)
			(*resources)[cnt++] = reg->resource;
		else
			break;
	}
	*nr_found = cnt;

	pthread_mutex_unlock(&sess->resources_lock);
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
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
	vaccel_warn("%s%s", "vaccel_sess_update() is deprecated. ",
		    "Please use vaccel_session_update() instead.");
	return vaccel_session_update(sess, flags);
#pragma GCC diagnostic pop
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
