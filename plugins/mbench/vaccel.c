// SPDX-License-Identifier: Apache-2.0

#include "vaccel.h"
#include <bits/time.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static struct vaccel_prof_region mbench_plugin_stats =
	VACCEL_PROF_REGION_INIT("vaccel_mbench_plugin");

#define NS_PER_SEC 1000000000L
#define NS_PER_MS 1000000L
#define MAX_TIME 300000

static int mbench(int time)
{
	int ret;
	int sec = 0;
	int usec = 1;
	struct timespec t;
	int64_t sts;
	int64_t ets;

	if (time > (int)MAX_TIME || time < 1)
		return VACCEL_EINVAL;

	clock_gettime(CLOCK_MONOTONIC_RAW, &t);
	sts = (int64_t)t.tv_sec * NS_PER_SEC + (int64_t)t.tv_nsec; // nsec
	while (1) {
		clock_gettime(CLOCK_MONOTONIC_RAW, &t);
		ets = (int64_t)t.tv_sec * NS_PER_SEC +
		      (int64_t)t.tv_nsec; // nsec
		if ((ets - sts) / NS_PER_MS >= (int64_t)time)
			break;
	}

	vaccel_debug("[mbench] %d ms elapsed", time);

	return VACCEL_OK;
}

static int mbench_unpack(struct vaccel_session *session, const char *library,
			 const char *fn_symbol, void *read, size_t nr_read,
			 void *write, size_t nr_write)
{
	(void)write;
	(void)nr_write;

	struct vaccel_arg *read_args = (struct vaccel_arg *)read;
	int time;
	int ret;

	vaccel_debug("Calling mbench for session %" PRId64, session->id);

	if (strcmp("mbench", library) != 0 && strcmp("mbench", fn_symbol) != 0)
		return VACCEL_EINVAL;

	if (nr_read < 2)
		return VACCEL_EINVAL;

	time = atoi(read_args[0].buf);

	vaccel_prof_region_start(&mbench_plugin_stats);

	ret = mbench(time);

	vaccel_prof_region_stop(&mbench_plugin_stats);

	return ret;
}

struct vaccel_op ops[] = {
	VACCEL_OP_INIT(ops[0], VACCEL_OP_EXEC, mbench_unpack),
};

static int init(void)
{
	return vaccel_plugin_register_ops(ops, sizeof(ops) / sizeof(ops[0]));
}

static int fini(void)
{
	vaccel_prof_region_print(&mbench_plugin_stats);
	vaccel_prof_region_release(&mbench_plugin_stats);

	return VACCEL_OK;
}

VACCEL_PLUGIN(.name = "mbench", .version = VACCEL_VERSION,
	      .vaccel_version = VACCEL_VERSION,
	      .type = VACCEL_PLUGIN_SOFTWARE | VACCEL_PLUGIN_GENERIC |
		      VACCEL_PLUGIN_CPU,
	      .init = init, .fini = fini)
