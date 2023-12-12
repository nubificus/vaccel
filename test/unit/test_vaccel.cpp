/*
 * The code below performs unit testing for vaccel.c
 *
 * This tests the constructor and destructor aspect of the vaccel.c file.
 * However the destructor/shutdown functions has issues with testing at the 
 * current amount/
 */

#include <atomic>
#include <catch.hpp>
#include <fff.h>

using atomic_int = std::atomic<int>;
using atomic_uint = std::atomic<unsigned int>;
DEFINE_FFF_GLOBALS;

extern "C" {
#include "log.h"
#include "plugin.h"
#include "resources.h"
#include "session.h"
#include "utils.h"

#include <dlfcn.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
}
TEST_CASE("constructor & destructor", "[vaccel]")
{   

    int ret; 
    ret = vaccel_log_init();
    REQUIRE(ret == VACCEL_OK);

    ret = sessions_bootstrap();
    REQUIRE(ret == VACCEL_OK);

    ret = resources_bootstrap();
    REQUIRE(ret == VACCEL_OK);

    ret = plugins_bootstrap();
    REQUIRE(ret == VACCEL_OK);

    // ret = plugins_shutdown();
    // REQUIRE(ret == VACCEL_OK);

    // ret = resources_cleanup();
    // REQUIRE(ret == VACCEL_OK);

    // ret = sessions_cleanup();
    // REQUIRE(ret == VACCEL_OK);

}
