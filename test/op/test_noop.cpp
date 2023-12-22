#include <catch.hpp>

#include <atomic>

using atomic_int = std::atomic<int>;
using atomic_uint = std::atomic<unsigned int>;

extern "C" {
#include <stdio.h>
#include <stdlib.h>
#include <vaccel.h>

#include "noop.h"
#include "session.h"
}

TEST_CASE("noop", "[noop]")
{

    int ret;
    struct vaccel_session sess;

    ret = vaccel_sess_init(&sess, 0);
    REQUIRE(ret == VACCEL_OK);

    ret = vaccel_noop(&sess);
    REQUIRE(ret == VACCEL_OK);

    ret = vaccel_sess_free(&sess);
    REQUIRE(ret == VACCEL_OK);
}