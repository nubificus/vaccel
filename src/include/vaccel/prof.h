// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "op.h"
#include <pthread.h>
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

	/* Lock that serializes concurrent access to the region's samples.
	 * The single-region API (start/stop/stop_with_context/print) is safe
	 * to call concurrently on the same region. The grouped *_by_name and
	 * print_all helpers assume the caller synchronizes access to the
	 * array. */
	pthread_mutex_t lock;
};

#define VACCEL_PROF_REGION_INIT(name) \
	{ (name), false, 0, NULL, 0, PTHREAD_MUTEX_INITIALIZER }

bool vaccel_prof_enabled(void);

/* Flush the active profiling backend before cleanup. */
int vaccel_prof_flush(void);

/* Start profiling a region */
int vaccel_prof_region_start(struct vaccel_prof_region *region);

/* Stop profiling a region */
int vaccel_prof_region_stop(struct vaccel_prof_region *region);

/* Stop profiling a region with operation context */
int vaccel_prof_region_stop_with_context(struct vaccel_prof_region *region,
					 vaccel_op_type_t op_type,
					 const char *plugin_name);

/* Print profiling results of a region */
int vaccel_prof_region_print(struct vaccel_prof_region *region);

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
