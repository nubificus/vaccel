/*
 *
 * The code below performs unit testing for bootstrapping and shutting down parts of the vAccel framework
 *
 *
 */

#include <catch.hpp>
#include <atomic>

using atomic_int = std::atomic<int>;
using atomic_uint = std::atomic<unsigned int>;

extern "C" {
#include "error.h"
#include "id_pool.h"
#include "list.h"
#include "log.h"
#include "plugin.h"
#include "resources.h"
#include "session.h"
#include "utils.h"
}

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define MAX_VACCEL_SESSIONS 1024
#define MAX_RESOURCES 2048
#define MAX_RESOURCE_RUNDIR 1024

TEST_CASE("plugin")
{

    int ret;
    ret = plugins_bootstrap();
    REQUIRE(ret == VACCEL_OK);

    ret = plugins_shutdown();
    REQUIRE(ret == VACCEL_OK);
}

TEST_CASE("resource")
{
    id_pool_t id_pool;
    list_t live_resources[VACCEL_RES_MAX];
    int ret;
    ret = id_pool_new(&id_pool, MAX_RESOURCES);
    REQUIRE(ret == VACCEL_OK);

    for (int i = 0; i < VACCEL_RES_MAX; ++i)
        list_init(&live_resources[i]);

    for (int i = 0; i < VACCEL_RES_MAX; ++i) {
        struct vaccel_resource *res, *tmp;
        for_each_vaccel_resource_safe(res, tmp, &live_resources[i])
            resource_destroy(res);
    }
    ret = id_pool_destroy(&id_pool);
    REQUIRE(ret == VACCEL_OK);
}

TEST_CASE("session")
{

    struct {
        bool initialized;
        id_pool_t ids;
        struct vaccel_session* running_sessions[MAX_VACCEL_SESSIONS];
    } sessions;

    int ret;
    ret = id_pool_new(&sessions.ids, MAX_VACCEL_SESSIONS);
    REQUIRE(ret == VACCEL_OK);

    for (size_t i = 0; i < MAX_VACCEL_SESSIONS; ++i)
        sessions.running_sessions[i] = NULL;

    sessions.initialized = true;

    ret = id_pool_destroy(&sessions.ids);
    REQUIRE(ret == VACCEL_OK);
    sessions.initialized = false;
}