// SPDX-License-Identifier: Apache-2.0

#define _POSIX_C_SOURCE 200809L

#include "backend.h"
#include "error.h"
#include "log.h"
#include "profiler.h"
#include <bits/time.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define NS_PER_SEC 1000000000L

enum { MIN_SAMPLES = 1024, MAX_NAME = 256 };

static uint64_t get_tstamp_nsec(void)
{
	struct timespec tp;

	clock_gettime(CLOCK_MONOTONIC_RAW, &tp);

	return (uint64_t)tp.tv_sec * NS_PER_SEC + (uint64_t)tp.tv_nsec;
}

static void profiler_sample_start(struct vaccel_profiler_sample *sample)
{
	sample->start = get_tstamp_nsec();
	sample->time = 0;
}

static void profiler_sample_stop(struct vaccel_profiler_sample *sample)
{
	sample->time = get_tstamp_nsec() - sample->start;
}

static int grow_samples_array(struct vaccel_profiler_region *region)
{
	size_t alloc_size = (region->size) ? region->size * 2 : MIN_SAMPLES;

	struct vaccel_profiler_sample *new_ptr =
		realloc(region->samples, alloc_size * sizeof(*new_ptr));
	if (!new_ptr)
		return VACCEL_ENOMEM;

	region->samples = new_ptr;
	region->size = alloc_size;

	return VACCEL_OK;
}

/* This will return that last used sample entry or NULL if no entries
 * have been used */
static struct vaccel_profiler_sample *
get_last_sample(const struct vaccel_profiler_region *region)
{
	size_t size = region->nr_entries;

	if (!size)
		return NULL;

	return &region->samples[size - 1];
}

/* Get next available profiler sample entry
 *
 * This will return the first unused sample entry. If needed it will
 * grow the capacity of the array */
static struct vaccel_profiler_sample *
get_next_sample(struct vaccel_profiler_region *region)
{
	size_t pos = region->nr_entries;

	/* The array is full. Try to grow it */
	if (pos >= region->size)
		if (grow_samples_array(region) != VACCEL_OK)
			return NULL;

	region->nr_entries++;
	return &region->samples[pos];
}

static int
vaccel_profiler_builtin_region_start(struct vaccel_profiler_region *region)
{
	vaccel_debug("Start profiler region %s", region->name);

	struct vaccel_profiler_sample *sample = get_next_sample(region);
	if (!sample)
		return VACCEL_ENOMEM;

	profiler_sample_start(sample);

	return VACCEL_OK;
}

static int
vaccel_profiler_builtin_region_stop(struct vaccel_profiler_region *region)
{
	vaccel_debug("Stop profiler region %s", region->name);

	struct vaccel_profiler_sample *sample = get_last_sample(region);
	if (!sample)
		return VACCEL_ENOENT;

	profiler_sample_stop(sample);

	return VACCEL_OK;
}

static int
vaccel_profiler_builtin_region_init(struct vaccel_profiler_region *region,
				    const char *name)
{
	if (name == NULL) {
		region->name = malloc(MAX_NAME);
		if (!region->name)
			return VACCEL_ENOMEM;
		memset((void *)region->name, 0, MAX_NAME);
	} else {
		region->name = strdup(name);
		if (!region->name)
			return VACCEL_ENOMEM;
	}

	region->name_owned = true;

	region->nr_entries = 0;
	region->samples = NULL;
	region->size = 0;
	if (grow_samples_array(region) != VACCEL_OK)
		goto free_name;

	return VACCEL_OK;

free_name:
	free((void *)region->name);
	return VACCEL_ENOMEM;
}

static int
vaccel_profiler_builtin_region_release(struct vaccel_profiler_region *region)
{
	if (region->samples)
		free(region->samples);

	if (region->name && region->name_owned)
		free((void *)region->name);

	region->name = NULL;
	region->name_owned = false;
	region->nr_entries = 0;
	region->samples = NULL;
	region->size = 0;

	return VACCEL_OK;
}

static int
vaccel_profiler_builtin_region_print(struct vaccel_profiler_region *region)
{
	if (!region->nr_entries)
		return VACCEL_OK;

	uint64_t total_time = 0;
	for (size_t i = 0; i < region->nr_entries; ++i)
		total_time += region->samples[i].time;

	uint64_t avg_time = total_time / region->nr_entries;
	double ops_per_sec = 0.0;

	if (total_time > 0) {
		ops_per_sec =
			((double)region->nr_entries * (double)NS_PER_SEC) /
			(double)total_time;
	}

	vaccel_info(
		"[profiler] %s: total_time: %lu nsec nr_entries: %lu avg_time: %lu nsec ops_per_sec: %.2f",
		region->name, total_time, region->nr_entries, avg_time,
		ops_per_sec);

	return VACCEL_OK;
}

static struct vaccel_profiler_region *
profiler_builtin_regions_get_by_name(struct vaccel_profiler_region *regions,
				     int nregions, const char *name)
{
	struct vaccel_profiler_region *r = NULL;
	for (int i = 0; i < nregions; i++) {
		if (strcmp(regions[i].name, name) == 0)
			r = &regions[i];
	}
	return r;
}

static int vaccel_profiler_builtin_regions_start_by_name(
	struct vaccel_profiler_region *regions, int nregions, const char *name)
{
	struct vaccel_profiler_region *r =
		profiler_builtin_regions_get_by_name(regions, nregions, name);
	if (!r) {
		vaccel_error("[profiler] stop region: Invalid profiler region");
		return VACCEL_EINVAL;
	}

	vaccel_debug("Start profiler region %s", r->name);

	struct vaccel_profiler_sample *sample = get_next_sample(r);
	if (!sample)
		return VACCEL_ENOMEM;

	profiler_sample_start(sample);

	return VACCEL_OK;
}

static int vaccel_profiler_builtin_regions_stop_by_name(
	struct vaccel_profiler_region *regions, int nregions, const char *name)
{
	struct vaccel_profiler_region *r =
		profiler_builtin_regions_get_by_name(regions, nregions, name);
	if (!r) {
		vaccel_error("[profiler] stop region: Invalid profiler region");
		return VACCEL_EINVAL;
	}

	vaccel_debug("Stop profiler region %s", r->name);

	struct vaccel_profiler_sample *sample = get_last_sample(r);
	if (!sample)
		return VACCEL_ENOENT;

	profiler_sample_stop(sample);

	return VACCEL_OK;
}

static int
vaccel_profiler_builtin_regions_release(struct vaccel_profiler_region *regions,
					int nregions)
{
	for (int i = 0; i < nregions; i++)
		vaccel_profiler_builtin_region_release(&regions[i]);

	return VACCEL_OK;
}

static int
vaccel_profiler_builtin_regions_init(struct vaccel_profiler_region *regions,
				     int nregions)
{
	for (int i = 0; i < nregions; i++) {
		int ret =
			vaccel_profiler_builtin_region_init(&regions[i], NULL);
		if (ret != VACCEL_OK) {
			vaccel_profiler_builtin_regions_release(regions, i);
			return ret;
		}
	}

	return VACCEL_OK;
}

static int vaccel_profiler_builtin_regions_print_all(
	struct vaccel_profiler_region *regions, int nregions)
{
	for (int i = 0; i < nregions; i++) {
		if (!regions[i].nr_entries)
			continue;

		uint64_t total_time = 0;
		for (size_t j = 0; j < regions[i].nr_entries; ++j)
			total_time += regions[i].samples[j].time;

		uint64_t avg_time = total_time / regions[i].nr_entries;
		double ops_per_sec = 0.0;

		if (total_time > 0) {
			ops_per_sec = ((double)regions[i].nr_entries *
				       (double)NS_PER_SEC) /
				      (double)total_time;
		}

		vaccel_info(
			"[profiler] %s: total_time: %lu nsec nr_entries: %lu avg_time: %lu nsec ops_per_sec: %.2f",
			regions[i].name, total_time, regions[i].nr_entries,
			avg_time, ops_per_sec);
	}

	return VACCEL_OK;
}

static int vaccel_profiler_builtin_regions_print_all_to_buf(
	char **tbuf, size_t tbuf_len, struct vaccel_profiler_region *regions,
	int size)
{
	int ssize = 0;
	int tsize = 0;
	int ret;

	uint64_t total_time[size];
	memset(total_time, 0, size * sizeof(uint64_t));
	for (int i = 0; i < size; i++) {
		if (!regions[i].nr_entries)
			continue;

		for (size_t j = 0; j < regions[i].nr_entries; ++j)
			total_time[i] += regions[i].samples[j].time;

		uint64_t avg_time = total_time[i] / regions[i].nr_entries;
		double ops_per_sec = 0.0;

		if (total_time[i] > 0) {
			ops_per_sec = ((double)regions[i].nr_entries *
				       (double)NS_PER_SEC) /
				      (double)total_time[i];
		}

		ssize +=
			snprintf(
				NULL, 0,
				"[profiler] %s: total_time: %ju nsec nr_entries: %zu avg_time: %ju nsec ops_per_sec: %.2f",
				regions[i].name, total_time[i],
				regions[i].nr_entries, avg_time, ops_per_sec) +
			1;
	}

	if (tbuf == NULL)
		return ssize;

	*tbuf = malloc(tbuf_len);
	if (!*tbuf)
		return -VACCEL_ENOMEM;

	for (int i = 0; i < size; i++) {
		if (!regions[i].nr_entries)
			continue;

		uint64_t avg_time = total_time[i] / regions[i].nr_entries;
		double ops_per_sec = 0.0;

		if (total_time[i] > 0) {
			ops_per_sec = ((double)regions[i].nr_entries *
				       (double)NS_PER_SEC) /
				      (double)total_time[i];
		}

		ret = snprintf(
			*tbuf + tsize, tbuf_len - tsize,
			"[profiler] %s: total_time: %ju nsec nr_entries: %zu avg_time: %ju nsec ops_per_sec: %.2f",
			regions[i].name, total_time[i], regions[i].nr_entries,
			avg_time, ops_per_sec);

		if (ret < 0) {
			free(*tbuf);
			return -VACCEL_EINVAL;
		}

		if (ret >= (int)(tbuf_len - tsize)) {
			free(*tbuf);
			return -VACCEL_ENOMEM;
		}

		tsize += ret;
	}

	return size;
}

static const struct vaccel_profiler_backend profiler_builtin_backend = {
	.region_start = vaccel_profiler_builtin_region_start,
	.region_stop = vaccel_profiler_builtin_region_stop,
	.region_stop_with_context = NULL,
	.region_init = vaccel_profiler_builtin_region_init,
	.region_release = vaccel_profiler_builtin_region_release,
	.region_print = vaccel_profiler_builtin_region_print,
	.regions_start_by_name = vaccel_profiler_builtin_regions_start_by_name,
	.regions_stop_by_name = vaccel_profiler_builtin_regions_stop_by_name,
	.regions_init = vaccel_profiler_builtin_regions_init,
	.regions_release = vaccel_profiler_builtin_regions_release,
	.regions_print_all = vaccel_profiler_builtin_regions_print_all,
	.regions_print_all_to_buf =
		vaccel_profiler_builtin_regions_print_all_to_buf,
};

const struct vaccel_profiler_backend *vaccel_profiler_builtin_backend_get(void)
{
	return &profiler_builtin_backend;
}
