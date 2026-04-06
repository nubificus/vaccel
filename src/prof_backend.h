// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "prof.h"

struct vaccel_prof_backend {
	int (*region_start)(struct vaccel_prof_region *region);
	int (*region_stop)(const struct vaccel_prof_region *region);
	int (*region_init)(struct vaccel_prof_region *region, const char *name);
	int (*region_release)(struct vaccel_prof_region *region);
	int (*region_print)(const struct vaccel_prof_region *region);

	int (*regions_start_by_name)(struct vaccel_prof_region *regions,
				     int nregions, const char *name);
	int (*regions_stop_by_name)(struct vaccel_prof_region *regions,
				    int nregions, const char *name);
	int (*regions_init)(struct vaccel_prof_region *regions, int nregions);
	int (*regions_release)(struct vaccel_prof_region *regions,
			       int nregions);
	int (*regions_print_all)(struct vaccel_prof_region *regions,
				 int nregions);
	int (*regions_print_all_to_buf)(char **tbuf, size_t tbuf_len,
					struct vaccel_prof_region *regions,
					int size);
};

const struct vaccel_prof_backend *vaccel_prof_backend_get(void);
const struct vaccel_prof_backend *vaccel_prof_base_backend_get(void);
