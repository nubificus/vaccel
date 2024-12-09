// SPDX-License-Identifier: Apache-2.0

#include "vaccel_prof.h"
#include "vaccel.h"
#include <bits/time.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

struct vaccel_prof_region mbench_plugin_stats =
	VACCEL_PROF_REGION_INIT("vaccel_mbench_plugin");

#define MAX_TIME 300000

static int mbench(int time)
{
	int ret;
	int sec = 0;
	int usec = 1;
	struct timespec t;
	uint64_t sts;
	uint64_t ets;

	if (time > (int)MAX_TIME || time < 1)
		return VACCEL_EINVAL;

	clock_gettime(CLOCK_MONOTONIC_RAW, &t);
	sts = t.tv_sec * 1e9 + t.tv_nsec; // nsec
	while (1) {
		clock_gettime(CLOCK_MONOTONIC_RAW, &t);
		ets = t.tv_sec * 1e9 + t.tv_nsec; // nsec
		if ((ets - sts) / 1000000 >= (uint64_t)time)
			break;
	}

	vaccel_debug("[mbench] %d ms elapsed", time);

	return VACCEL_OK;
}

static int mbench_unpack(struct vaccel_session *session, const char *library,
			 const char *fn_symbol, void *read, size_t nr_read,
			 void *write, size_t nr_write)
{
	struct vaccel_arg *read_args = (struct vaccel_arg *)read;
	int time;
	int ret;
	//void *buf;

	vaccel_debug("Calling mbench for session %" PRId64, session->id);

	if (strcmp("mbench", library) != 0 && strcmp("mbench", fn_symbol) != 0)
		return VACCEL_EINVAL;

	if (nr_read < 2)
		return VACCEL_EINVAL;

	time = atoi(read_args[0].buf);
	//buf = read_args[1].buf;

	vaccel_prof_region_start(&mbench_plugin_stats);

	ret = mbench(time);

	vaccel_prof_region_stop(&mbench_plugin_stats);

	return ret;
}
struct vaccel_op ops[] = {
	VACCEL_OP_INIT(ops[0], VACCEL_EXEC, mbench_unpack),
};

static int init(void)
{
	return register_plugin_functions(ops, sizeof(ops) / sizeof(ops[0]));
}

static int fini(void)
{
	vaccel_prof_region_print(&mbench_plugin_stats);

	return VACCEL_OK;
}

VACCEL_MODULE(.name = "mbench", .version = VACCEL_VERSION,
	      .vaccel_version = VACCEL_VERSION,
	      .type = VACCEL_PLUGIN_SOFTWARE | VACCEL_PLUGIN_GENERIC |
		      VACCEL_PLUGIN_CPU,
	      .init = init, .fini = fini)
