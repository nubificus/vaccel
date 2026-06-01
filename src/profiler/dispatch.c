// SPDX-License-Identifier: Apache-2.0

#include "backend.h"
#include "config.h"
#include "core.h"
#include "error.h"
#include "log.h"
#include "op.h"
#include "profiler.h"
#include <pthread.h>
#include <stddef.h>

bool vaccel_profiler_enabled(void)
{
	const struct vaccel_config *config = vaccel_config();

	return config->profiling_enabled;
}

int vaccel_profiler_region_start(struct vaccel_profiler_region *region)
{
	if (!vaccel_profiler_enabled())
		return VACCEL_OK;

	if (!region) {
		vaccel_error(
			"[profiler] start region: Invalid profiler region");
		return VACCEL_EINVAL;
	}

	pthread_mutex_lock(&region->lock);
	int ret = vaccel_profiler_backend_get()->region_start(region);
	pthread_mutex_unlock(&region->lock);

	return ret;
}

int vaccel_profiler_region_stop(struct vaccel_profiler_region *region)
{
	if (!vaccel_profiler_enabled())
		return VACCEL_OK;

	if (!region) {
		vaccel_error("[profiler] stop region: Invalid profiler region");
		return VACCEL_EINVAL;
	}

	pthread_mutex_lock(&region->lock);
	int ret = vaccel_profiler_backend_get()->region_stop(region);
	pthread_mutex_unlock(&region->lock);

	return ret;
}

int vaccel_profiler_region_stop_with_context(
	struct vaccel_profiler_region *region, vaccel_op_type_t op_type,
	const char *plugin_name)

{
	if (!vaccel_profiler_enabled())
		return VACCEL_OK;

	if (!region) {
		vaccel_error("[profiler] stop region: Invalid profiler region");
		return VACCEL_EINVAL;
	}

	const struct vaccel_profiler_backend *backend =
		vaccel_profiler_backend_get();

	pthread_mutex_lock(&region->lock);
	int ret = backend->region_stop_with_context ?
			  backend->region_stop_with_context(region, op_type,
							    plugin_name) :
			  backend->region_stop(region);
	pthread_mutex_unlock(&region->lock);

	return ret;
}

int vaccel_profiler_region_init(struct vaccel_profiler_region *region,
				const char *name)
{
	if (!vaccel_profiler_enabled())
		return VACCEL_OK;

	if (!region) {
		vaccel_error("[profiler] init region: Invalid profiler region");
		return VACCEL_EINVAL;
	}

	if (pthread_mutex_init(&region->lock, NULL)) {
		vaccel_error(
			"[profiler] init region: Could not initialize lock");
		return VACCEL_EINVAL;
	}

	int ret = vaccel_profiler_backend_get()->region_init(region, name);
	if (ret)
		pthread_mutex_destroy(&region->lock);

	return ret;
}

int vaccel_profiler_region_release(struct vaccel_profiler_region *region)
{
	if (!vaccel_profiler_enabled())
		return VACCEL_OK;

	if (!region) {
		vaccel_error(
			"[profiler] release region: Invalid profiler region");
		return VACCEL_EINVAL;
	}

	int ret = vaccel_profiler_backend_get()->region_release(region);
	pthread_mutex_destroy(&region->lock);

	return ret;
}

int vaccel_profiler_region_print(struct vaccel_profiler_region *region)
{
	if (!vaccel_profiler_enabled())
		return VACCEL_OK;

	if (!region) {
		vaccel_error(
			"[profiler] print region: Invalid profiler region");
		return VACCEL_EINVAL;
	}

	pthread_mutex_lock(&region->lock);
	int ret = vaccel_profiler_backend_get()->region_print(region);
	pthread_mutex_unlock(&region->lock);

	return ret;
}

int vaccel_profiler_regions_start_by_name(
	struct vaccel_profiler_region *regions, int nregions, const char *name)
{
	if (!vaccel_profiler_enabled())
		return VACCEL_OK;

	if (!regions) {
		vaccel_error(
			"[profiler] start region: Invalid profiler region array");
		return VACCEL_EINVAL;
	}

	return vaccel_profiler_backend_get()->regions_start_by_name(
		regions, nregions, name);
}

int vaccel_profiler_regions_stop_by_name(struct vaccel_profiler_region *regions,
					 int nregions, const char *name)
{
	if (!vaccel_profiler_enabled())
		return VACCEL_OK;

	if (!regions) {
		vaccel_error(
			"[profiler] stop region: Invalid profiler region array");
		return VACCEL_EINVAL;
	}

	return vaccel_profiler_backend_get()->regions_stop_by_name(
		regions, nregions, name);
}

int vaccel_profiler_regions_init(struct vaccel_profiler_region *regions,
				 int nregions)
{
	if (!vaccel_profiler_enabled())
		return VACCEL_OK;

	if (!regions) {
		vaccel_error(
			"[profiler] init regions: Invalid profiler region array");
		return VACCEL_EINVAL;
	}

	for (int i = 0; i < nregions; i++) {
		if (pthread_mutex_init(&regions[i].lock, NULL)) {
			vaccel_error(
				"[profiler] init regions: Could not initialize lock");
			for (int j = 0; j < i; j++)
				pthread_mutex_destroy(&regions[j].lock);
			return VACCEL_EINVAL;
		}
	}

	int ret =
		vaccel_profiler_backend_get()->regions_init(regions, nregions);
	if (ret)
		for (int i = 0; i < nregions; i++)
			pthread_mutex_destroy(&regions[i].lock);

	return ret;
}

int vaccel_profiler_regions_release(struct vaccel_profiler_region *regions,
				    int nregions)
{
	if (!vaccel_profiler_enabled())
		return VACCEL_OK;

	if (!regions) {
		vaccel_error(
			"[profiler] release regions: Invalid profiler region array");
		return VACCEL_EINVAL;
	}

	int ret = vaccel_profiler_backend_get()->regions_release(regions,
								 nregions);
	for (int i = 0; i < nregions; i++)
		pthread_mutex_destroy(&regions[i].lock);

	return ret;
}

int vaccel_profiler_regions_print_all(struct vaccel_profiler_region *regions,
				      int nregions)
{
	if (!vaccel_profiler_enabled())
		return VACCEL_OK;

	if (!regions) {
		vaccel_error(
			"[profiler] print regions: Invalid profiler region array");
		return VACCEL_EINVAL;
	}

	return vaccel_profiler_backend_get()->regions_print_all(regions,
								nregions);
}

int vaccel_profiler_regions_print_all_to_buf(
	char **tbuf, size_t tbuf_len, struct vaccel_profiler_region *regions,
	int size)
{
	if (!vaccel_profiler_enabled())
		return 0;

	if (!regions) {
		vaccel_error(
			"[profiler] print regions: Invalid profiler region array");
		return -VACCEL_EINVAL;
	}

	return vaccel_profiler_backend_get()->regions_print_all_to_buf(
		tbuf, tbuf_len, regions, size);
}
