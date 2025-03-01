// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct vaccel_prof_sample {
	/* Timestamp (nsec) of entering the region */
	uint64_t start;

	/* Time (nsec) elapsed inside the region */
	uint64_t time;
};

struct vaccel_prof_region {
	/* Name of the region */
	const char *name;

	/* 'true' if we own the memory of 'name' */
	bool name_owned;

	/* Number of collected samples */
	size_t nr_entries;

	/* Array of collected samples */
	struct vaccel_prof_sample *samples;

	/* Allocated size for the array */
	size_t size;
};

#define VACCEL_PROF_REGION_INIT(name) { (name), false, 0, NULL, 0 }

bool vaccel_prof_enabled(void);

/* Start profiling a region */
int vaccel_prof_region_start(struct vaccel_prof_region *region);

/* Stop profiling a region */
int vaccel_prof_region_stop(const struct vaccel_prof_region *region);

/* Print profiling results of a region */
int vaccel_prof_region_print(const struct vaccel_prof_region *region);

/* Initialize a profiling region */
int vaccel_prof_region_init(struct vaccel_prof_region *region,
			    const char *name);

/* Destroy a profiling region */
int vaccel_prof_region_release(struct vaccel_prof_region *region);

/* Start profiling a region by name from an array of regions */
int vaccel_prof_regions_start_by_name(struct vaccel_prof_region *regions,
				      int nregions, const char *name);

/* Stop profiling a region by name from an array of regions */
int vaccel_prof_regions_stop_by_name(struct vaccel_prof_region *regions,
				     int nregions, const char *name);

/* Initialize an array of profiling regions */
int vaccel_prof_regions_init(struct vaccel_prof_region *regions, int nregions);

/* Release data of an array of profiling regions */
int vaccel_prof_regions_release(struct vaccel_prof_region *regions,
				int nregions);

/* Print profiling results of an array of regions */
int vaccel_prof_regions_print_all(struct vaccel_prof_region *regions,
				  int nregions);

/* Print profiling results of an array of regions to a buffer */
int vaccel_prof_regions_print_all_to_buf(char **tbuf, size_t tbuf_len,
					 struct vaccel_prof_region *regions,
					 int size);

#ifdef __cplusplus
}
#endif
