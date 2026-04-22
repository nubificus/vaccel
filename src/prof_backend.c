// SPDX-License-Identifier: Apache-2.0

#include "prof_backend.h"
#include "core.h"
#include "error.h"
#include "log.h"
#include <pthread.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#define PROF_BACKEND_BASE_NAME "base"
#define PROF_BACKENDS_MAX 4

struct prof_backend_entry {
	const char *name;
	const struct vaccel_prof_backend *backend;
};

static struct {
	bool initialized;
	struct prof_backend_entry entries[PROF_BACKENDS_MAX];
	size_t count;
	/* Name of the last unknown backend we warned about, so we don't
	 * spam the log on every region call */
	char *warned_unknown;
	pthread_mutex_t lock;
} prof_backends;

int prof_backends_bootstrap(void)
{
	if (prof_backends.initialized)
		return VACCEL_OK;

	pthread_mutex_init(&prof_backends.lock, NULL);
	prof_backends.count = 0;
	prof_backends.warned_unknown = NULL;

	prof_backends.entries[0].name = PROF_BACKEND_BASE_NAME;
	prof_backends.entries[0].backend = vaccel_prof_base_backend_get();
	prof_backends.count = 1;

	prof_backends.initialized = true;
	return VACCEL_OK;
}

int prof_backends_cleanup(void)
{
	if (!prof_backends.initialized)
		return VACCEL_OK;

	pthread_mutex_lock(&prof_backends.lock);
	prof_backends.count = 0;
	if (prof_backends.warned_unknown) {
		free(prof_backends.warned_unknown);
		prof_backends.warned_unknown = NULL;
	}
	pthread_mutex_unlock(&prof_backends.lock);

	pthread_mutex_destroy(&prof_backends.lock);
	prof_backends.initialized = false;
	return VACCEL_OK;
}

static const struct vaccel_prof_backend *
prof_backend_find_locked(const char *name)
{
	for (size_t i = 0; i < prof_backends.count; i++) {
		if (strcmp(prof_backends.entries[i].name, name) == 0)
			return prof_backends.entries[i].backend;
	}
	return NULL;
}

int vaccel_plugin_register_prof_backend(
	const char *name, const struct vaccel_prof_backend *backend)
{
	if (!name || !backend)
		return VACCEL_EINVAL;

	if (strcmp(name, PROF_BACKEND_BASE_NAME) == 0) {
		vaccel_error("[prof] '%s' is reserved for the built-in backend",
			     PROF_BACKEND_BASE_NAME);
		return VACCEL_EINVAL;
	}

	if (!prof_backends.initialized)
		return VACCEL_EBACKEND;

	pthread_mutex_lock(&prof_backends.lock);

	if (prof_backend_find_locked(name)) {
		pthread_mutex_unlock(&prof_backends.lock);
		vaccel_error("[prof] backend '%s' already registered", name);
		return VACCEL_EEXIST;
	}

	if (prof_backends.count >= PROF_BACKENDS_MAX) {
		pthread_mutex_unlock(&prof_backends.lock);
		vaccel_error("[prof] too many backends registered (max %d)",
			     PROF_BACKENDS_MAX);
		return VACCEL_ENOMEM;
	}

	prof_backends.entries[prof_backends.count].name = name;
	prof_backends.entries[prof_backends.count].backend = backend;
	prof_backends.count++;

	/* Clear stale warning for this backend. */
	if (prof_backends.warned_unknown &&
	    strcmp(prof_backends.warned_unknown, name) == 0) {
		free(prof_backends.warned_unknown);
		prof_backends.warned_unknown = NULL;
	}

	pthread_mutex_unlock(&prof_backends.lock);

	vaccel_debug("[prof] Registered backend '%s'", name);
	return VACCEL_OK;
}

int vaccel_plugin_unregister_prof_backend(const char *name)
{
	if (!name)
		return VACCEL_EINVAL;

	if (strcmp(name, PROF_BACKEND_BASE_NAME) == 0)
		return VACCEL_EINVAL;

	if (!prof_backends.initialized)
		return VACCEL_EBACKEND;

	pthread_mutex_lock(&prof_backends.lock);

	for (size_t i = 0; i < prof_backends.count; i++) {
		if (strcmp(prof_backends.entries[i].name, name) != 0)
			continue;

		prof_backends.count--;
		if (i != prof_backends.count)
			prof_backends.entries[i] =
				prof_backends.entries[prof_backends.count];

		pthread_mutex_unlock(&prof_backends.lock);
		vaccel_debug("[prof] Unregistered backend '%s'", name);
		return VACCEL_OK;
	}

	pthread_mutex_unlock(&prof_backends.lock);
	return VACCEL_ENOENT;
}

const struct vaccel_prof_backend *vaccel_prof_backend_get(void)
{
	const struct vaccel_config *config = vaccel_config();

	if (!config || !config->profiling_enabled || !prof_backends.initialized)
		return vaccel_prof_base_backend_get();

	const char *requested = config->profiling_backend ?
					config->profiling_backend :
					PROF_BACKEND_BASE_NAME;
	
	pthread_mutex_lock(&prof_backends.lock);

	const struct vaccel_prof_backend *found =
		prof_backend_find_locked(requested);
	if (found) {
		pthread_mutex_unlock(&prof_backends.lock);
		return found;
	}

	/* Unknown / not-yet-loaded backend: warn once per name. */
	bool should_warn = !prof_backends.warned_unknown ||
			   strcmp(prof_backends.warned_unknown, requested) != 0;
	if (should_warn) {
		size_t len = strlen(requested) + 1;

		if (prof_backends.warned_unknown) {
			free(prof_backends.warned_unknown);
		}

		prof_backends.warned_unknown = malloc(len);
		if (prof_backends.warned_unknown)
			memcpy(prof_backends.warned_unknown, requested, len);
	}

	pthread_mutex_unlock(&prof_backends.lock);

	if (should_warn)
		vaccel_warn(
			"[prof] backend '%s' not registered, falling back to '%s'",
			requested, PROF_BACKEND_BASE_NAME);

	return vaccel_prof_base_backend_get();
}
