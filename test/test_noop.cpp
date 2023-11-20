#define CATCH_CONFIG_MAIN
#include <catch2/catch_test_macros.hpp>

#include <atomic>

using atomic_int = std::atomic<int>;
using atomic_uint = std::atomic<unsigned int>;

extern "C"
{
#include <vaccel.h>
#include <stdlib.h>
#include <stdio.h>

#include "session.h"
#include "noop.h"

}

TEST_CASE("noop", "[noop]") {

    int ret;
	struct vaccel_session sess;

    ret = vaccel_sess_init(&sess, 0);
    REQUIRE(ret == VACCEL_OK);

    ret = vaccel_noop(&sess);
    REQUIRE(ret == VACCEL_OK);

    ret = vaccel_sess_free(&sess);
    REQUIRE(ret == VACCEL_OK);

}