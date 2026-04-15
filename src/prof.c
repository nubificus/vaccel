// SPDX-License-Identifier: Apache-2.0

#include "prof.h"
#include "prof_backend.h"
#include "config.h"
#include "core.h"
#include "error.h"

bool vaccel_prof_enabled(void)
{
	const struct vaccel_config *config = vaccel_config();

	return config->profiling_enabled;
}

int vaccel_prof_region_start(struct vaccel_prof_region *region)
{
	if (!vaccel_prof_enabled())
		return VACCEL_OK;

	if (!region) {
		vaccel_error("[prof] start region: Invalid profiling region");
		return VACCEL_EINVAL;
	}

	return vaccel_prof_backend_get()->region_start(region);
}

int vaccel_prof_region_stop(const struct vaccel_prof_region *region)
{
	if (!vaccel_prof_enabled())
		return VACCEL_OK;

	if (!region) {
		vaccel_error("[prof] stop region: Invalid profiling region");
		return VACCEL_EINVAL;
	}

	return vaccel_prof_backend_get()->region_stop(region);
}

int vaccel_prof_region_stop_with_context(const struct vaccel_prof_region *region,
					 vaccel_op_type_t op_type,
					 const char *plugin_name)

{
	if (!vaccel_prof_enabled())
		return VACCEL_OK;

	if (!region) {
		vaccel_error("[prof] init region: Invalid profiling region");
		return VACCEL_EINVAL;
	}

	const struct vaccel_prof_backend *backend = vaccel_prof_backend_get();

	if (!backend->region_stop_with_context)
		return backend->region_stop(region);

	return backend->region_stop_with_context(region, op_type, plugin_name);
}

int vaccel_prof_region_init(struct vaccel_prof_region *region, const char *name)
{
	if (!vaccel_prof_enabled())
		return VACCEL_OK;

	if (!region) {
		vaccel_error("[prof] init region: Invalid profiling region");
		return VACCEL_EINVAL;
	}

	return vaccel_prof_backend_get()->region_init(region, name);
}

int vaccel_prof_region_release(struct vaccel_prof_region *region)
{
	if (!vaccel_prof_enabled())
		return VACCEL_OK;

	if (!region) {
		vaccel_error("[prof] release region: Invalid profiling region");
		return VACCEL_EINVAL;
	}

	return vaccel_prof_backend_get()->region_release(region);
}

int vaccel_prof_region_print(const struct vaccel_prof_region *region)
{
	if (!vaccel_prof_enabled())
		return VACCEL_OK;

	if (!region) {
		vaccel_error("[prof] print region: Invalid profiling region");
		return VACCEL_EINVAL;
	}

	return vaccel_prof_backend_get()->region_print(region);
}

int vaccel_prof_regions_start_by_name(struct vaccel_prof_region *regions,
				      int nregions, const char *name)
{
	if (!vaccel_prof_enabled())
		return VACCEL_OK;

	if (!regions) {
		vaccel_error(
			"[prof] start region: Invalid profiling region array");
		return VACCEL_EINVAL;
	}

	return vaccel_prof_backend_get()->regions_start_by_name(regions,
								nregions, name);
}

int vaccel_prof_regions_stop_by_name(struct vaccel_prof_region *regions,
				     int nregions, const char *name)
{
	if (!vaccel_prof_enabled())
		return VACCEL_OK;

	if (!regions) {
		vaccel_error(
			"[prof] stop region: Invalid profiling region array");
		return VACCEL_EINVAL;
	}

	return vaccel_prof_backend_get()->regions_stop_by_name(regions,
							       nregions, name);
}

int vaccel_prof_regions_init(struct vaccel_prof_region *regions, int nregions)
{
	if (!vaccel_prof_enabled())
		return VACCEL_OK;

	if (!regions) {
		vaccel_error(
			"[prof] init regions: Invalid profiling region array");
		return VACCEL_EINVAL;
	}

	return vaccel_prof_backend_get()->regions_init(regions, nregions);
}

int vaccel_prof_regions_release(struct vaccel_prof_region *regions,
				int nregions)
{
	if (!vaccel_prof_enabled())
		return VACCEL_OK;

	if (!regions) {
		vaccel_error(
			"[prof] release regions: Invalid profiling region array");
		return VACCEL_EINVAL;
	}

	return vaccel_prof_backend_get()->regions_release(regions, nregions);
}

int vaccel_prof_regions_print_all(struct vaccel_prof_region *regions,
				  int nregions)
{
	if (!vaccel_prof_enabled())
		return VACCEL_OK;

	if (!regions) {
		vaccel_error(
			"[prof] print regions: Invalid profiling region array");
		return VACCEL_EINVAL;
	}

	return vaccel_prof_backend_get()->regions_print_all(regions, nregions);
}

int vaccel_prof_regions_print_all_to_buf(char **tbuf, size_t tbuf_len,
					 struct vaccel_prof_region *regions,
					 int size)
{
	if (!vaccel_prof_enabled())
		return 0;

	if (!regions) {
		vaccel_error(
			"[prof] print regions: Invalid profiling region array");
		return -VACCEL_EINVAL;
	}

	return vaccel_prof_backend_get()->regions_print_all_to_buf(
		tbuf, tbuf_len, regions, size);
}
