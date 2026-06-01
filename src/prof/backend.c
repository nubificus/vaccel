// SPDX-License-Identifier: Apache-2.0

#include "backend.h"
#include "config.h"
#include "core.h"
#include "error.h"
#include "log.h"
#include "plugin.h"
#include <stdatomic.h>
#include <string.h>

static _Atomic(const struct vaccel_prof_backend *) active_backend;

/* A backend must implement every hook except the optional
 * region_stop_with_context and flush. */
static bool prof_backend_valid(const struct vaccel_prof_backend *backend)
{
	return backend->region_start && backend->region_stop &&
	       backend->region_init && backend->region_release &&
	       backend->region_print && backend->regions_start_by_name &&
	       backend->regions_stop_by_name && backend->regions_init &&
	       backend->regions_release && backend->regions_print_all &&
	       backend->regions_print_all_to_buf;
}

static void prof_backend_set(const struct vaccel_prof_backend *backend)
{
	atomic_store_explicit(&active_backend, backend, memory_order_release);
}

int prof_backends_bootstrap(void)
{
	prof_backend_set(vaccel_prof_builtin_backend_get());
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
		prof_backend_set(vaccel_prof_builtin_backend_get());
		return VACCEL_OK;
	}

	const char *requested = config->profiling_backend ?
					config->profiling_backend :
					VACCEL_PROF_BACKEND_BUILTIN_NAME;

	if (strcmp(requested, VACCEL_PROF_BACKEND_BUILTIN_NAME) == 0) {
		prof_backend_set(vaccel_prof_builtin_backend_get());
		return VACCEL_OK;
	}

	struct vaccel_plugin *plugin = plugin_find_by_name(requested);
	if (!plugin || !plugin->info || !plugin->info->prof_backend) {
		vaccel_warn(
			"[prof] plugin '%s' not found, falling back to '%s'",
			requested, VACCEL_PROF_BACKEND_BUILTIN_NAME);
		prof_backend_set(vaccel_prof_builtin_backend_get());
		return VACCEL_OK;
	}

	if (!prof_backend_valid(plugin->info->prof_backend)) {
		vaccel_warn(
			"[prof] backend '%s' is missing required hooks, falling back to '%s'",
			requested, VACCEL_PROF_BACKEND_BUILTIN_NAME);
		prof_backend_set(vaccel_prof_builtin_backend_get());
		return VACCEL_OK;
	}

	prof_backend_set(plugin->info->prof_backend);
	return VACCEL_OK;
}

const struct vaccel_prof_backend *vaccel_prof_backend_get(void)
{
	const struct vaccel_prof_backend *backend =
		atomic_load_explicit(&active_backend, memory_order_acquire);

	return backend ? backend : vaccel_prof_builtin_backend_get();
}

int vaccel_prof_flush(void)
{
	const struct vaccel_prof_backend *backend = vaccel_prof_backend_get();

	if (!backend || !backend->flush)
		return VACCEL_OK;

	return backend->flush();
}
