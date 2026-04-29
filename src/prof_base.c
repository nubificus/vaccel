// SPDX-License-Identifier: Apache-2.0

#define _POSIX_C_SOURCE 200809L

#include "prof_backend.h"
#include "error.h"
#include "log.h"
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

static void prof_sample_start(struct vaccel_prof_sample *sample)
{
	sample->start = get_tstamp_nsec();
	sample->time = 0;
}

static void prof_sample_stop(struct vaccel_prof_sample *sample)
{
	sample->time = get_tstamp_nsec() - sample->start;
}

static int grow_samples_array(struct vaccel_prof_region *region)
{
	size_t alloc_size = (region->size) ? region->size * 2 : MIN_SAMPLES;

	struct vaccel_prof_sample *new_ptr =
		realloc(region->samples, alloc_size * sizeof(*new_ptr));
	if (!new_ptr)
		return VACCEL_ENOMEM;

	region->samples = new_ptr;
	region->size = alloc_size;

	return VACCEL_OK;
}

/* This will return that last used sample entry or NULL if no entries
 * have been used */
static struct vaccel_prof_sample *
get_last_sample(const struct vaccel_prof_region *region)
{
	size_t size = region->nr_entries;

	if (!size)
		return NULL;

	return &region->samples[size - 1];
}

/* Get next available profiling sample entry
 *
 * This will return the first unused sample entry. If needed it will
 * grow the capacity of the array */
static struct vaccel_prof_sample *
get_next_sample(struct vaccel_prof_region *region)
{
	size_t pos = region->nr_entries;

	/* The array is full. Try to grow it */
	if (pos >= region->size)
		if (grow_samples_array(region) != VACCEL_OK)
			return NULL;

	region->nr_entries++;
	return &region->samples[pos];
}

static int vaccel_prof_base_region_start(struct vaccel_prof_region *region)
{
	vaccel_debug("Start profiling region %s", region->name);

	struct vaccel_prof_sample *sample = get_next_sample(region);
	if (!sample)
		return VACCEL_ENOMEM;

	prof_sample_start(sample);

	return VACCEL_OK;
}

static int vaccel_prof_base_region_stop(const struct vaccel_prof_region *region)
{
	vaccel_debug("Stop profiling region %s", region->name);

	struct vaccel_prof_sample *sample = get_last_sample(region);
	if (!sample)
		return VACCEL_ENOENT;

	prof_sample_stop(sample);

	return VACCEL_OK;
}

static int vaccel_prof_base_region_init(struct vaccel_prof_region *region,
					const char *name)
{
	if (name == NULL) {
		region->name = malloc(MAX_NAME);
		if (!region->name)
			return VACCEL_ENOMEM;
		memset((void *)region->name, 0, MAX_NAME);
	} else if (name) {
		region->name = strdup(name);
		if (!region->name)
			return VACCEL_ENOMEM;
	} else {
		vaccel_error("[prof] init region: Invalid region name");
		return VACCEL_EINVAL;
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

static int vaccel_prof_base_region_release(struct vaccel_prof_region *region)
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
vaccel_prof_base_region_print(const struct vaccel_prof_region *region)
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
		"[prof] %s: total_time: %lu nsec nr_entries: %lu avg_time: %lu nsec ops_per_sec: %.2f",
		region->name, total_time, region->nr_entries, avg_time,
		ops_per_sec);

	return VACCEL_OK;
}

static struct vaccel_prof_region *
prof_base_regions_get_by_name(struct vaccel_prof_region *regions, int nregions,
			      const char *name)
{
	struct vaccel_prof_region *r = NULL;
	for (int i = 0; i < nregions; i++) {
		if (strcmp(regions[i].name, name) == 0)
			r = &regions[i];
	}
	return r;
}

static int
vaccel_prof_base_regions_start_by_name(struct vaccel_prof_region *regions,
				       int nregions, const char *name)
{
	struct vaccel_prof_region *r =
		prof_base_regions_get_by_name(regions, nregions, name);
	if (!r) {
		vaccel_error("[prof] stop region: Invalid profiling region");
		return VACCEL_EINVAL;
	}

	vaccel_debug("Start profiling region %s", r->name);

	struct vaccel_prof_sample *sample = get_next_sample(r);
	if (!sample)
		return VACCEL_ENOMEM;

	prof_sample_start(sample);

	return VACCEL_OK;
}

static int
vaccel_prof_base_regions_stop_by_name(struct vaccel_prof_region *regions,
				      int nregions, const char *name)
{
	struct vaccel_prof_region *r =
		prof_base_regions_get_by_name(regions, nregions, name);
	if (!r) {
		vaccel_error("[prof] stop region: Invalid profiling region");
		return VACCEL_EINVAL;
	}

	vaccel_debug("Stop profiling region %s", r->name);

	struct vaccel_prof_sample *sample = get_last_sample(r);
	if (!sample)
		return VACCEL_ENOENT;

	prof_sample_stop(sample);

	return VACCEL_OK;
}

static int vaccel_prof_base_regions_release(struct vaccel_prof_region *regions,
					    int nregions)
{
	for (int i = 0; i < nregions; i++) {
		free(regions[i].samples);
		regions[i].samples = NULL;
		regions[i].size = 0;
	}

	return VACCEL_OK;
}

static int vaccel_prof_base_regions_init(struct vaccel_prof_region *regions,
					 int nregions)
{
	for (int i = 0; i < nregions; i++) {
		regions[i].size = 0;
		regions[i].samples = NULL;
		int ret = vaccel_prof_base_region_init(&regions[i], NULL);
		if (ret != VACCEL_OK) {
			vaccel_prof_base_regions_release(regions, i);
			return ret;
		}
	}

	return VACCEL_OK;
}

static int
vaccel_prof_base_regions_print_all(struct vaccel_prof_region *regions,
				   int nregions)
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
			"[prof] %s: total_time: %lu nsec nr_entries: %lu avg_time: %lu nsec ops_per_sec: %.2f",
			regions[i].name, total_time, regions[i].nr_entries,
			avg_time, ops_per_sec);
	}

	return VACCEL_OK;
}

static int
vaccel_prof_base_regions_print_all_to_buf(char **tbuf, size_t tbuf_len,
					  struct vaccel_prof_region *regions,
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
				"[prof] %s: total_time: %ju nsec nr_entries: %zu avg_time: %ju nsec ops_per_sec: %.2f",
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
			"[prof] %s: total_time: %ju nsec nr_entries: %zu avg_time: %ju nsec ops_per_sec: %.2f",
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
		(*tbuf)[tsize] = '\n';
		tsize++;
	}

	return size;
}

static const struct vaccel_prof_backend prof_base_backend = {
	.region_start = vaccel_prof_base_region_start,
	.region_stop = vaccel_prof_base_region_stop,
	.region_stop_with_context = NULL,
	.region_init = vaccel_prof_base_region_init,
	.region_release = vaccel_prof_base_region_release,
	.region_print = vaccel_prof_base_region_print,
	.regions_start_by_name = vaccel_prof_base_regions_start_by_name,
	.regions_stop_by_name = vaccel_prof_base_regions_stop_by_name,
	.regions_init = vaccel_prof_base_regions_init,
	.regions_release = vaccel_prof_base_regions_release,
	.regions_print_all = vaccel_prof_base_regions_print_all,
	.regions_print_all_to_buf = vaccel_prof_base_regions_print_all_to_buf,
};

const struct vaccel_prof_backend *vaccel_prof_base_backend_get(void)
{
	return &prof_base_backend;
}
