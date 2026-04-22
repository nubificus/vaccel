// SPDX-License-Identifier: Apache-2.0

#include "prof_backend.h"
#include "core.h"
#include "error.h"
#include "log.h"
#include "plugin.h"

#include <stdatomic.h>
#include <string.h>

#define PROF_BACKEND_BUILTIN_NAME "builtin"

static _Atomic(const struct vaccel_prof_backend *) active_backend;

static void prof_backend_set(const struct vaccel_prof_backend *backend)
{
	atomic_store_explicit(&active_backend, backend, memory_order_release);
}

int prof_backends_bootstrap(void)
{
	prof_backend_set(vaccel_prof_base_backend_get());
	return VACCEL_OK;
}

int prof_backends_cleanup(void)
{
	prof_backend_set(NULL);
	return VACCEL_OK;
}

int prof_backend_select(void)
{
	const struct vaccel_config *config = vaccel_config();

	if (!config || !config->profiling_enabled) {
		prof_backend_set(vaccel_prof_base_backend_get());
		return VACCEL_OK;
	}

	const char *requested = config->profiling_backend ?
					config->profiling_backend :
					PROF_BACKEND_BUILTIN_NAME;

	if (strcmp(requested, PROF_BACKEND_BUILTIN_NAME) == 0) {
		prof_backend_set(vaccel_prof_base_backend_get());
		return VACCEL_OK;
	}

	struct vaccel_plugin *plugin = plugin_find_by_name(requested);
	if (!plugin || !plugin->info || !plugin->info->prof_backend) {
		vaccel_warn(
			"[prof] plugin '%s' not found, falling back to '%s'",
			requested, PROF_BACKEND_BUILTIN_NAME);
		prof_backend_set(vaccel_prof_base_backend_get());
		return VACCEL_OK;
	}

	prof_backend_set(plugin->info->prof_backend);
	return VACCEL_OK;
}

const struct vaccel_prof_backend *vaccel_prof_backend_get(void)
{
	const struct vaccel_prof_backend *backend =
		atomic_load_explicit(&active_backend, memory_order_acquire);

	return backend ? backend : vaccel_prof_base_backend_get();
}

int vaccel_prof_flush(void)
{
	const struct vaccel_prof_backend *backend = vaccel_prof_backend_get();

	if (!backend || !backend->flush)
		return VACCEL_OK;

	return backend->flush();
}
