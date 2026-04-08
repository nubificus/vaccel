// SPDX-License-Identifier: Apache-2.0

#include "prof_backend.h"
#include "core.h"
#include "log.h"
#include <string.h>

const struct vaccel_prof_backend *vaccel_prof_backend_get(void)
{
	const struct vaccel_config *config = vaccel_config();

	if (!config || !config->profiling_enabled)
    		return vaccel_prof_base_backend_get();
	
	if (!config || !config->profiling_backend)
		return vaccel_prof_base_backend_get();

	if (strcmp(config->profiling_backend, "otel") == 0)
		return vaccel_prof_otel_backend_get();

	if (strcmp(config->profiling_backend, "base") == 0)
		return vaccel_prof_base_backend_get();

	vaccel_warn("[prof] Unknown profiling backend '%s', using base",
		    config->profiling_backend);
	
	return vaccel_prof_base_backend_get();
}
