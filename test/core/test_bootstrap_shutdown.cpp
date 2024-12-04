// SPDX-License-Identifier: Apache-2.0

/*
 * The code below performs unit testing for bootstrapping and shutting down
 * parts of the vAccel framework
 *
 */

#include "vaccel.h"
#include <catch.hpp>
#include <cstdio>
#include <cstdlib>
#include <cstring>

enum { MAX_SESSIONS = 1024, MAX_RESOURCES = 2048, MAX_RESOURCE_RUNDIR = 1024 };

TEST_CASE("bootstrap_plugin", "[core][bootstrap]")
{
	int ret;
	ret = plugins_bootstrap();
	REQUIRE(ret == VACCEL_OK);

	ret = plugins_shutdown();
	REQUIRE(ret == VACCEL_OK);
}

TEST_CASE("bootstrap_resource", "[core][bootstrap]")
{
	id_pool_t id_pool;
	vaccel_list_t live_resources[VACCEL_RESOURCE_MAX];
	int ret;
	ret = id_pool_init(&id_pool, MAX_RESOURCES);
	REQUIRE(ret == VACCEL_OK);

	for (auto &live_resource : live_resources)
		list_init(&live_resource);

	for (auto &live_resource : live_resources) {
		struct vaccel_resource *res;
		struct vaccel_resource *tmp;
		resource_for_each_safe(res, tmp, &live_resource)
			vaccel_resource_release(res);
	}
	ret = id_pool_release(&id_pool);
	REQUIRE(ret == VACCEL_OK);
}

TEST_CASE("bootstrap_session", "[core][bootstrap]")
{
	struct {
		bool initialized;
		id_pool_t ids;
		struct vaccel_session *running_sessions[MAX_SESSIONS];
	} sessions;

	int ret;
	ret = id_pool_init(&sessions.ids, MAX_SESSIONS);
	REQUIRE(ret == VACCEL_OK);

	for (auto &running_session : sessions.running_sessions)
		running_session = nullptr;

	sessions.initialized = true;

	ret = id_pool_release(&sessions.ids);
	REQUIRE(ret == VACCEL_OK);
	sessions.initialized = false;
}
