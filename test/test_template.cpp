
#include <catch.hpp>

#include <atomic>

using atomic_int = std::atomic<int>;
using atomic_uint = std::atomic<unsigned int>;

extern "C" {
#include "error.h"
}

TEST_CASE("template", "[temp]")
{

    int ret = 0;
    REQUIRE(ret == VACCEL_OK);
}