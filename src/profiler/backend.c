// SPDX-License-Identifier: Apache-2.0

#include "backend.h"
#include "config.h"
#include "core.h"
#include "error.h"
#include "log.h"
#include "plugin.h"
#include <stdatomic.h>
#include <string.h>

static _Atomic(const struct vaccel_profiler_backend *) active_backend;

/* A backend must implement every hook except the optional
 * region_stop_with_context and flush. */
static bool
profiler_backend_valid(const struct vaccel_profiler_backend *backend)
{
	return backend->region_start && backend->region_stop &&
	       backend->region_init && backend->region_release &&
	       backend->region_print && backend->regions_start_by_name &&
	       backend->regions_stop_by_name && backend->regions_init &&
	       backend->regions_release && backend->regions_print_all &&
	       backend->regions_print_all_to_buf;
}

static void profiler_backend_set(const struct vaccel_profiler_backend *backend)
{
	atomic_store_explicit(&active_backend, backend, memory_order_release);
}

int profiler_backends_bootstrap(void)
{
	profiler_backend_set(vaccel_profiler_builtin_backend_get());
	return VACCEL_OK;
}

int profiler_backends_cleanup(void)
{
	profiler_backend_set(NULL);
	return VACCEL_OK;
}

int profiler_backend_select(void)
{
	const struct vaccel_config *config = vaccel_config();

	if (!config || !config->profiling_enabled) {
		profiler_backend_set(vaccel_profiler_builtin_backend_get());
		return VACCEL_OK;
	}

	const char *requested = config->profiler_backend ?
					config->profiler_backend :
					VACCEL_PROFILER_BACKEND_BUILTIN_NAME;

	if (strcmp(requested, VACCEL_PROFILER_BACKEND_BUILTIN_NAME) == 0) {
		profiler_backend_set(vaccel_profiler_builtin_backend_get());
		return VACCEL_OK;
	}

	struct vaccel_plugin *plugin = plugin_find_by_name(requested);
	if (!plugin || !plugin->info || !plugin->info->profiler_backend) {
		vaccel_warn(
			"[profiler] plugin '%s' not found, falling back to '%s'",
			requested, VACCEL_PROFILER_BACKEND_BUILTIN_NAME);
		profiler_backend_set(vaccel_profiler_builtin_backend_get());
		return VACCEL_OK;
	}

	if (!profiler_backend_valid(plugin->info->profiler_backend)) {
		vaccel_warn(
			"[profiler] backend '%s' is missing required hooks, falling back to '%s'",
			requested, VACCEL_PROFILER_BACKEND_BUILTIN_NAME);
		profiler_backend_set(vaccel_profiler_builtin_backend_get());
		return VACCEL_OK;
	}

	profiler_backend_set(plugin->info->profiler_backend);
	return VACCEL_OK;
}

const struct vaccel_profiler_backend *vaccel_profiler_backend_get(void)
{
	const struct vaccel_profiler_backend *backend =
		atomic_load_explicit(&active_backend, memory_order_acquire);

	return backend ? backend : vaccel_profiler_builtin_backend_get();
}

int vaccel_profiler_flush(void)
{
	const struct vaccel_profiler_backend *backend =
		vaccel_profiler_backend_get();

	if (!backend || !backend->flush)
		return VACCEL_OK;

	return backend->flush();
}
