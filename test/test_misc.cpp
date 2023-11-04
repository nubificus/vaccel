#include <catch2/catch_test_macros.hpp>
#include <fff.h>
DEFINE_FFF_GLOBALS;

extern "C"{
#include "misc.h"
FAKE_VALUE_FUNC(int, get_available_plugins, enum vaccel_op_type);
}

TEST_CASE("vaccel_get_plugins", "[vaccel_get_plugins]")
{

    struct vaccel_session session;
    session.session_id = 123;

    SECTION("return correct implementation")
    {
        get_available_plugins_fake.return_val = 15;
        int result = vaccel_get_plugins(&session, VACCEL_NO_OP);
        REQUIRE(result == 15);
    }

    SECTION("no session available")
    {
        REQUIRE(vaccel_get_plugins(NULL, VACCEL_NO_OP) == VACCEL_EINVAL);
    }


}