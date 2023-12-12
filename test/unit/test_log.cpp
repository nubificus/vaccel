/*
 * The code below performs unit testing to log.
 *
 * 1) vaccel_log_init()
 * 2) vaccel_log_shutdown()
 *
 */

#include <catch.hpp>

extern "C" {
#include "error.h"
#include "log.h"

#include <slog.h>
#include <stdlib.h>
#include <string.h>
}

TEST_CASE("log level 1", "[log]")
{
    char env_var[] = "VACCEL_DEBUG_LEVEL=1";
    putenv(env_var);
    int ret = vaccel_log_init();
    REQUIRE(ret == VACCEL_OK);
    ret = vaccel_log_shutdown();
    REQUIRE(ret == VACCEL_OK);
}
TEST_CASE("log level 2", "[log]")
{
    char env_var[] = "VACCEL_DEBUG_LEVEL=2";
    putenv(env_var);
    int ret = vaccel_log_init();
    REQUIRE(ret == VACCEL_OK);
    ret = vaccel_log_shutdown();
    REQUIRE(ret == VACCEL_OK);
}
TEST_CASE("log level 3", "[log]")
{
    char env_var[] = "VACCEL_DEBUG_LEVEL=3";
    putenv(env_var);
    int ret = vaccel_log_init();
    REQUIRE(ret == VACCEL_OK);
    ret = vaccel_log_shutdown();
    REQUIRE(ret == VACCEL_OK);
}
TEST_CASE("log level 4", "[log]")
{
    char env_var[] = "VACCEL_DEBUG_LEVEL=4";
    putenv(env_var);
    int ret = vaccel_log_init();
    REQUIRE(ret == VACCEL_OK);
    ret = vaccel_log_shutdown();
    REQUIRE(ret == VACCEL_OK);
}
