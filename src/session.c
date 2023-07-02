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

#include "session.h"
#include "plugin.h"
#include "log.h"
#include "utils.h"
#include "id_pool.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>

#define MAX_VACCEL_SESSIONS 1024

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
	id_pool_destroy(&sessions.ids);
	sessions.initialized = false;

	return VACCEL_OK;
}

int vaccel_sess_register(struct vaccel_session *sess,
		struct vaccel_resource *res)
{
	if (!sess || !sess->resources || !res || res->type >= VACCEL_RES_MAX)
		return VACCEL_EINVAL;

	struct session_resources *resources = sess->resources;
	struct registered_resource *container = malloc(sizeof(*container));
	if (!container)
		return VACCEL_ENOMEM;

	struct vaccel_plugin *plugin = get_virtio_plugin();
	if (plugin) {
		int ret = plugin->info->sess_register(sess->session_id, res->id);
		if (ret) {
			free(container);
			return ret;
		}
	}

	container->res = res;
	list_add_tail(&resources->registered[res->type], &container->entry);
	resource_refcount_inc(res);

	return VACCEL_OK;
}

static struct registered_resource *
find_registered_resource(struct vaccel_session *sess,
		struct vaccel_resource *res)
{
	struct session_resources *resources = sess->resources;
	list_t *list = &resources->registered[res->type];

	struct registered_resource *iter;
	for_each_session_resource(iter, list) {
		if (iter->res == res)
			return iter;
	}

	return NULL;
}

int vaccel_sess_unregister(struct vaccel_session *sess,
		struct vaccel_resource *res)
{
	if (!sess || !res || res->type >= VACCEL_RES_MAX)
		return VACCEL_EINVAL;

	/* Check if resource is indeed registered to session */
	struct registered_resource *container =
		find_registered_resource(sess, res);
	if (!container) {
		vaccel_warn("Resource %u not registered with session %u\n",
				res->id, sess->session_id);
		return VACCEL_EINVAL;
	}

	struct vaccel_plugin *plugin = get_virtio_plugin();
	if (plugin) {
		int ret = plugin->info->sess_unregister(sess->session_id, res->id);
		if (ret) {
			vaccel_error("BUG: Could not unregister host-side resource %u",
					res->id);
		}
	}

	list_unlink_entry(&container->entry);
	resource_refcount_dec(container->res);
	free(container);

	return VACCEL_OK;
}

bool vaccel_sess_has_resource(
	struct vaccel_session *sess,
	struct vaccel_resource *res
) {
	return find_registered_resource(sess, res) != NULL;
}

static int initialize_session_resources(struct vaccel_session *sess)
{
	if (!sess)
		return VACCEL_EINVAL;

	struct session_resources *res = malloc(sizeof(*res));
	if (!res)
		return VACCEL_ENOMEM;

	const char *root_rundir = vaccel_rundir();
	int ret = snprintf(res->rundir, MAX_SESSION_RUNDIR_PATH,
			"%s/session.%u", root_rundir, sess->session_id);
	if (ret == MAX_SESSION_RUNDIR_PATH) {
		vaccel_error("rundir path '%s/session.%u' too big",
				root_rundir, sess->session_id);
		ret = VACCEL_ENAMETOOLONG;
		goto cleanup_res;
	}

	ret = mkdir(res->rundir, 0700);
	if (ret)
		goto cleanup_res;

	sess->resources = res;
	for (int i = 0; i < VACCEL_RES_MAX; ++i)
		list_init(&res->registered[i]);

	return VACCEL_OK;

cleanup_res:
	free(res);
	return ret;
}

static int cleanup_session_resources(struct vaccel_session *sess)
{
	if (!sess || !sess->resources)
		return VACCEL_EINVAL;

	struct session_resources *resources = sess->resources;
	for (int i = 0; i < VACCEL_RES_MAX; ++i) {
		struct registered_resource *iter, *tmp;
		for_each_session_resource_safe(iter, tmp, &resources->registered[i]) {
			int ret = vaccel_sess_unregister(sess, iter->res);
			if (ret)
				vaccel_error("Could not unregister resource from session");
		}
	}

	/* Try to cleanup the rundir. At the moment, we do not fail
	 * if this fails, we just warn the user */
	int ret = cleanup_rundir(sess->resources->rundir);
	if (ret)
		vaccel_warn("Could not cleanup rundir '%s' for session %u",
				sess->resources->rundir, sess->session_id);

	free(sess->resources);
	sess->resources = NULL;

	return VACCEL_OK;
}

int vaccel_sess_init(struct vaccel_session *sess, uint32_t flags)
{
	if (!sess)
		return VACCEL_EINVAL;

	if (!sessions.initialized)
		return VACCEL_ESESS;

	/* if we're using virtio as a plugin offload the session initialization
	 * to the host */
	struct vaccel_plugin *virtio = get_virtio_plugin();
	if (virtio) {
		int ret = virtio->info->sess_init(sess, flags);
		if (ret) {
			vaccel_warn("Could not create host-side session");
			return ret;
		}
	} else {
		uint32_t sess_id = get_sess_id();
		if (!sess_id)
			return VACCEL_ESESS;

		sess->session_id = sess_id;
	}

	int ret = initialize_session_resources(sess);
	if (ret)
		goto cleanup_session;

	sess->hint = flags;

	vaccel_debug("session:%u New session", sess->session_id);

	sessions.running_sessions[sess->session_id - 1] = sess;

	return VACCEL_OK;

cleanup_session:
	if (virtio) {
		int ret = virtio->info->sess_free(sess);
		if (ret) {
			vaccel_error("BUG: Could not cleanup host-side session");

		}
	} else {
		put_sess_id(sess->session_id);
	}

	return ret;
}

int vaccel_sess_update(struct vaccel_session *sess, uint32_t flags)
{
	if (!sess)
		return VACCEL_EINVAL;

	if (!sessions.initialized)
		return VACCEL_ESESS;

	/* if we're using virtio as a plugin offload the session update to the
	 * host */
	struct vaccel_plugin *virtio = get_virtio_plugin();
	if (virtio) {
		int ret = virtio->info->sess_update(sess, flags);
		if (ret) {
			vaccel_warn("Could not update host-side session");
		}
	} else {
		sess->hint = flags;
	}

	vaccel_debug("session:%u update with flags: %u", sess->session_id, flags);

	return VACCEL_OK;
}

int vaccel_sess_free(struct vaccel_session *sess)
{
	if (!sess)
		return VACCEL_EINVAL;

	if (!sessions.initialized)
		return VACCEL_ESESS;

	/* if we're using virtio as a plugin offload the session cleanup to the
	 * host */
	struct vaccel_plugin *virtio = get_virtio_plugin();
	if (virtio) {
		int ret = virtio->info->sess_free(sess);
		if (ret) {
			vaccel_warn("Could not cleanup host-side session");
		}
	} else {
		put_sess_id(sess->session_id);
	}

	cleanup_session_resources(sess);
	sessions.running_sessions[sess->session_id - 1] = NULL;

	vaccel_debug("session:%u Free session", sess->session_id);

	return VACCEL_OK;
}
