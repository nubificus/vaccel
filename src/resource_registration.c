// SPDX-License-Identifier: Apache-2.0

#include "resource_registration.h"
#include "error.h"
#include "list.h"
#include "log.h"
#include "resource.h"
#include "session.h"
#include <inttypes.h>
#include <limits.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int resource_registration_new(struct resource_registration **reg,
			      struct vaccel_resource *res,
			      struct vaccel_session *sess)
{
	if (!reg || !res || !sess)
		return VACCEL_EINVAL;

	*reg = malloc(sizeof(**reg));
	if (!*reg)
		return VACCEL_ENOMEM;

	(*reg)->resource = res;
	(*reg)->session = sess;

	return VACCEL_OK;
}

int resource_registration_delete(struct resource_registration *reg)
{
	if (!reg)
		return VACCEL_EINVAL;

	free(reg);
	return VACCEL_OK;
}

int resource_registration_link(struct resource_registration *reg)
{
	if (!reg || !reg->resource || !reg->session)
		return VACCEL_EINVAL;

	struct vaccel_resource *res = reg->resource;
	struct vaccel_session *sess = reg->session;

	pthread_mutex_lock(&res->sessions_lock);
	pthread_mutex_lock(&sess->resources_lock);

	list_add_tail(&res->sessions, &reg->resource_entry);
	list_add_tail(&sess->resources[res->type], &reg->session_entry);
	sess->resource_counts[res->type]++;

	atomic_fetch_add(&res->refcount, 1);

	pthread_mutex_unlock(&sess->resources_lock);
	pthread_mutex_unlock(&res->sessions_lock);

	return VACCEL_OK;
}

static int
resource_registration_unlink_unlocked(struct resource_registration *reg)
{
	if (!reg || !reg->resource || !reg->session)
		return VACCEL_EINVAL;

	struct vaccel_resource *res = reg->resource;
	struct vaccel_session *sess = reg->session;

	list_unlink_entry(&reg->resource_entry);
	list_unlink_entry(&reg->session_entry);
	sess->resource_counts[res->type]--;

	atomic_fetch_sub(&res->refcount, 1);

	return VACCEL_OK;
}

int resource_registration_unlink(struct resource_registration *reg)
{
	if (!reg || !reg->resource || !reg->session)
		return VACCEL_EINVAL;

	struct vaccel_resource *res = reg->resource;
	struct vaccel_session *sess = reg->session;

	pthread_mutex_lock(&res->sessions_lock);
	pthread_mutex_lock(&sess->resources_lock);

	resource_registration_unlink_unlocked(reg);

	pthread_mutex_unlock(&sess->resources_lock);
	pthread_mutex_unlock(&res->sessions_lock);

	return VACCEL_OK;
}

struct resource_registration *
resource_registration_find(struct vaccel_resource *res,
			   struct vaccel_session *sess)
{
	if (!res || !sess)
		return NULL;

	pthread_mutex_lock(&res->sessions_lock);

	struct resource_registration *reg;
	list_for_each_container(reg, &res->sessions,
				struct resource_registration, resource_entry)
	{
		if (reg->session == sess) {
			pthread_mutex_unlock(&res->sessions_lock);
			return reg;
		}
	}

	pthread_mutex_unlock(&res->sessions_lock);
	return NULL;
}

struct resource_registration *
resource_registration_find_and_unlink(struct vaccel_resource *res,
				      struct vaccel_session *sess)
{
	if (!res || !sess)
		return NULL;

	pthread_mutex_lock(&res->sessions_lock);

	struct resource_registration *reg;
	struct resource_registration *tmp;
	list_for_each_container_safe(reg, tmp, &res->sessions,
				     struct resource_registration,
				     resource_entry)
	{
		if (reg->session == sess) {
			pthread_mutex_lock(&sess->resources_lock);
			int ret = resource_registration_unlink_unlocked(reg);
			pthread_mutex_unlock(&sess->resources_lock);
			if (ret) {
				pthread_mutex_unlock(&res->sessions_lock);
				return NULL;
			}

			pthread_mutex_unlock(&res->sessions_lock);
			return reg;
		}
	}

	pthread_mutex_unlock(&res->sessions_lock);
	return NULL;
}

int resource_registration_foreach_session(
	struct vaccel_resource *res,
	int (*callback)(struct vaccel_resource *res,
			struct vaccel_session *sess))
{
	if (!res || !callback)
		return VACCEL_EINVAL;

	// Collect all sessions first to avoid iterator invalidation during
	// callback
	size_t nr_sessions = atomic_load(&res->refcount);
	if (nr_sessions == 0)
		return VACCEL_OK;

	struct vaccel_session **sessions = (struct vaccel_session **)malloc(
		nr_sessions * sizeof(struct vaccel_session *));
	if (!sessions)
		return VACCEL_ENOMEM;

	pthread_mutex_lock(&res->sessions_lock);

	size_t tmp_nr_sessions = atomic_load(&res->refcount);
	if (nr_sessions < tmp_nr_sessions) {
		struct vaccel_session **tmp_sessions =
			(struct vaccel_session **)realloc(
				(void *)sessions,
				tmp_nr_sessions *
					sizeof(struct vaccel_session *));
		if (!tmp_sessions) {
			pthread_mutex_unlock(&res->sessions_lock);
			free(sessions);
			return VACCEL_ENOMEM;
		}
		sessions = tmp_sessions;
	}
	nr_sessions = tmp_nr_sessions;

	struct resource_registration *reg;
	size_t cnt = 0;
	list_for_each_container(reg, &res->sessions,
				struct resource_registration, resource_entry)
	{
		if (cnt >= nr_sessions) {
			pthread_mutex_unlock(&res->sessions_lock);
			free(sessions);
			vaccel_error("BUG: Registration count mismatch");
			return VACCEL_EINVAL;
		}
		sessions[cnt++] = reg->session;
	}

	pthread_mutex_unlock(&res->sessions_lock);

	// Call callback for each session
	for (size_t i = 0; i < cnt; i++) {
		int ret = callback(res, sessions[i]);
		if (ret) {
			free(sessions);
			return ret;
		}
	}

	free(sessions);
	return VACCEL_OK;
}

int resource_registration_foreach_resource(
	struct vaccel_session *sess,
	int (*callback)(struct vaccel_resource *res,
			struct vaccel_session *sess))
{
	if (!sess || !callback)
		return VACCEL_EINVAL;

	// Collect all resources first to avoid iterator invalidation during
	// callback
	pthread_mutex_lock(&sess->resources_lock);
	size_t nr_resources = 0;
	for (size_t type = 0; type < VACCEL_RESOURCE_MAX; type++)
		nr_resources += sess->resource_counts[type];
	pthread_mutex_unlock(&sess->resources_lock);

	if (nr_resources == 0)
		return VACCEL_OK;

	struct vaccel_resource **resources = (struct vaccel_resource **)malloc(
		nr_resources * sizeof(struct vaccel_resource *));
	if (!resources)
		return VACCEL_ENOMEM;

	pthread_mutex_lock(&sess->resources_lock);

	size_t tmp_nr_resources = 0;
	for (size_t type = 0; type < VACCEL_RESOURCE_MAX; type++)
		tmp_nr_resources += sess->resource_counts[type];

	if (nr_resources < tmp_nr_resources) {
		struct vaccel_resource **tmp_resources =
			(struct vaccel_resource **)realloc(
				(void *)resources,
				tmp_nr_resources *
					sizeof(struct vaccel_resource *));
		if (!tmp_resources) {
			pthread_mutex_unlock(&sess->resources_lock);
			free(resources);
			return VACCEL_ENOMEM;
		}
		resources = tmp_resources;
	}
	nr_resources = tmp_nr_resources;

	size_t cnt = 0;
	for (int type = 0; type < VACCEL_RESOURCE_MAX; type++) {
		struct resource_registration *reg;
		list_for_each_container(reg, &sess->resources[type],
					struct resource_registration,
					session_entry)
		{
			if (cnt >= nr_resources) {
				pthread_mutex_unlock(&sess->resources_lock);
				free(resources);
				vaccel_error(
					"BUG: Registration count mismatch");
				return VACCEL_EINVAL;
			}
			resources[cnt++] = reg->resource;
		}
	}

	pthread_mutex_unlock(&sess->resources_lock);

	// Call callback for each resource
	for (size_t i = 0; i < cnt; i++) {
		int ret = callback(resources[i], sess);
		if (ret) {
			free(resources);
			return ret;
		}
	}

	free(resources);
	return VACCEL_OK;
}
