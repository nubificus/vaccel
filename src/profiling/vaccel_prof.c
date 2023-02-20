/*
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "vaccel_prof.h"

#include "log.h"
#include "error.h"

#include <malloc.h>
#include <stdint.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <time.h>

#define MIN_SAMPLES 1024

uint64_t get_tstamp_nsec(void)
{
	struct timespec tp;

	clock_gettime(CLOCK_MONOTONIC_RAW, &tp);

	return tp.tv_sec * 1e9 + tp.tv_nsec;
}

bool vaccel_prof_enabled(void)
{
	char *env = getenv("VACCEL_PROF_ENABLED");

	return env && !strncmp(env, "enabled", 7);
}

struct prof_sample {
	/* Timestamp (nsec) of entering the region */
	uint64_t tstamp_start;

	/* Timestamp (nsec) of exiting the region */
	uint64_t tstamp_end;
};

void prof_sample_start(struct prof_sample *sample)
{
	sample->tstamp_start = get_tstamp_nsec();
}

void prof_sample_stop(struct prof_sample *sample)
{
	sample->tstamp_end = get_tstamp_nsec();
}

uint64_t prof_sample_duration(struct prof_sample *sample)
{
	return sample->tstamp_end - sample->tstamp_start;
}

static int grow_samples_array(struct vaccel_prof_region *region)
{
	size_t alloc_size = (region->size) ? region->size * 2 : MIN_SAMPLES;

	struct prof_sample *new_ptr =
		realloc(region->samples, alloc_size * sizeof(*new_ptr));
	if (!new_ptr)
		return VACCEL_ENOMEM;

	region->samples = new_ptr;
	region->size = alloc_size;

	return VACCEL_OK;
}

/* This will return that last used sample entry or NULL if no entries
 * have been used */
static struct prof_sample *get_last_sample(
	const struct vaccel_prof_region *region
) {
	size_t size = region->nr_entries;

	if (!size)
		return NULL;

	return &region->samples[size - 1];
}

/* Get next available profiling sample entry
 *
 * This will return the first unused sample entry. If needed it will
 * grow the capacity of the array */
static struct prof_sample *get_next_sample(struct vaccel_prof_region *region)
{
	size_t pos = region->nr_entries;

	/* The array is full. Try to grow it */
	if (pos >= region->size)
		if (grow_samples_array(region) != VACCEL_OK)
			return NULL;

	region->nr_entries++;
	return &region->samples[pos];
}

int vaccel_prof_region_start(struct vaccel_prof_region *region)
{
	if (!vaccel_prof_enabled())
		return VACCEL_OK;

	if (!region) {
		vaccel_error("[prof] start region: Invalid profiling region");
		return VACCEL_EINVAL;
	}

	vaccel_debug("Start profiling region %s", region->name);

	struct prof_sample *sample = get_next_sample(region);
	if (!sample)
		return VACCEL_ENOMEM;

	prof_sample_start(sample);

	return VACCEL_OK;
}

int vaccel_prof_region_stop(const struct vaccel_prof_region *region)
{
	if (!vaccel_prof_enabled())
		return VACCEL_OK;

	if (!region) {
		vaccel_error("[prof] stop region: Invalid profiling region");
		return VACCEL_EINVAL;
	}

	vaccel_debug("Stop profiling region %s", region->name);

	struct prof_sample *sample = get_last_sample(region);
	if (!sample)
		return VACCEL_ENOENT;

	prof_sample_stop(sample);

	return VACCEL_OK;
}

int vaccel_prof_region_init(
	struct vaccel_prof_region *region,
	const char *name
) {
	if (!vaccel_prof_enabled())
		return VACCEL_OK;

	if (!region) {
		vaccel_error("[prof] init region: Invalid profiling region");
		return VACCEL_EINVAL;
	}

	if (!name) {
		vaccel_error("[prof] init region: Invalid region name");
		return VACCEL_EINVAL;
	}

	region->name = strdup(name);
	if (!region->name)
		return VACCEL_ENOMEM;

	region->name_owned = true;

	region->nr_entries = 0;
	if (grow_samples_array(region) != VACCEL_OK)
		goto free_name;

	return VACCEL_OK;
	
free_name:
	free((void *)region->name);
	return VACCEL_ENOMEM;
}

int vaccel_prof_region_destroy(struct vaccel_prof_region *region)
{
	if (!vaccel_prof_enabled())
		return VACCEL_OK;

	if (!region) {
		vaccel_error("[prof] destroy region: Invalid profiling region");
		return VACCEL_EINVAL;
	}

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

int vaccel_prof_region_print(const struct vaccel_prof_region *region)
{
	if (!vaccel_prof_enabled())
		return VACCEL_OK;

	if (!region) {
		vaccel_error("[prof] print region: Invalid profiling region");
		return VACCEL_EINVAL;
	}

	if (!region->nr_entries)
		return VACCEL_OK;

	uint64_t total_time = 0;
	for (size_t i = 0; i < region->nr_entries; ++i)
		total_time += prof_sample_duration(&region->samples[i]);

	vaccel_info("[prof] %s: total_time: %lu nsec nr_entries: %lu",
			region->name, total_time, region->nr_entries);

	return VACCEL_OK;
}
