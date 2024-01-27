/*
 * Unit Testing for VAccel FPGA Plugins
 *
 * The code below performs unit testing for VAccel FPGA plugins.
 * It includes test cases for the `vaccel_fpga_add`, `vaccel_fpga_copy`,
 * `vaccel_fpga_mmult`, and `vaccel_fpga_parallel` functions.
 *
 */

#include <catch.hpp>
#include <iostream>
#include <utils.hpp>

extern "C" {
#include <vaccel.h>
}

TEST_CASE("vaccel_fpga_add", "[vaccel_fpga_plugins]")
{
    int ret;
    struct vaccel_session sess;
    sess.session_id = 0;
    sess.priv = nullptr;
    sess.resources = nullptr;
    sess.hint = 1;

    // Ensure that the session system is initialized
    ret = sessions_bootstrap();
    REQUIRE(ret == VACCEL_OK);

    float A[] = { 1, 2, 3, 4, 5 };
    float B[] = { 1, 2, 6, 4, 5 };
    float C[] = { 1, 2, 6, 4, 5 };

    size_t len_A = sizeof(A) / sizeof(A[0]);
    size_t len_B = sizeof(B) / sizeof(B[0]);

    const char* vaccel_backends = std::getenv("VACCEL_BACKENDS");

    if (vaccel_backends) {
        std::cout << "VACCEL_BACKENDS: " << vaccel_backends << std::endl;
    } else {
        std::cerr << "VACCEL_BACKENDS environment variable not set." << std::endl;
    }

    SECTION("null session")
    {
        REQUIRE(vaccel_fpga_vadd(NULL, A, B, C, len_A, len_B) == VACCEL_EINVAL);
    }

    SECTION("valid session and inputs")
    {
        ret = vaccel_sess_init(&sess, 0);
        REQUIRE(ret == VACCEL_OK);
        REQUIRE(sess.session_id);
        REQUIRE(sess.hint == 0);
        REQUIRE(sess.resources);
        REQUIRE(sess.priv == nullptr);

        ret = vaccel_fpga_vadd(&sess, A, B, C, len_A, len_B);
        float C_expected[] = { 2, 4, 9, 8, 10 };
        size_t len_C = sizeof(C) / sizeof(C[0]);
        size_t len_C_expected = sizeof(C_expected) / sizeof(C_expected[0]);

        REQUIRE(vaccel_sess_free(&sess) == VACCEL_OK);
        REQUIRE(ret != VACCEL_ENOTSUP);

        REQUIRE(len_C == len_C_expected);

        REQUIRE(std::equal(C, C + len_C, C_expected));
    }

    ret = sessions_cleanup();
    REQUIRE(ret == VACCEL_OK);
}

TEST_CASE("vaccel_fpga_copy", "[vaccel_fpga_plugins]")
{
    int ret;
    struct vaccel_session sess;
    sess.session_id = 0;
    sess.priv = nullptr;
    sess.resources = nullptr;
    sess.hint = 1;

    // Ensure that the session system is initialized
    ret = sessions_bootstrap();
    REQUIRE(ret == VACCEL_OK);

    int A[] = { 1, 2, 3, 4, 5 };
    int B[] = { 1, 1, 1, 1, 1 };

    size_t len_A = sizeof(A) / sizeof(A[0]);
    size_t len_B = sizeof(B) / sizeof(B[0]);

    SECTION("null session")
    {
        REQUIRE(vaccel_fpga_arraycopy(NULL, A, B, len_A) == VACCEL_EINVAL);
    }

    SECTION("valid session and inputs")
    {
        ret = vaccel_sess_init(&sess, 0);
        REQUIRE(ret == VACCEL_OK);
        REQUIRE(sess.session_id);
        REQUIRE(sess.hint == 0);
        REQUIRE(sess.resources);
        REQUIRE(sess.priv == nullptr);
        ret = vaccel_fpga_arraycopy(&sess, A, B, len_A);
        float B_expected[] = { 1, 2, 3, 4, 5 };

        REQUIRE(vaccel_sess_free(&sess) == VACCEL_OK);
        REQUIRE(ret != VACCEL_ENOTSUP);

        REQUIRE(std::equal(B, B + len_B, B_expected));
    }

    ret = sessions_cleanup();
    REQUIRE(ret == VACCEL_OK);
}

TEST_CASE("vaccel_fpga_mmult", "[vaccel_fpga_plugins]")
{
    int ret;
    struct vaccel_session sess;
    sess.session_id = 0;
    sess.priv = nullptr;
    sess.resources = nullptr;
    sess.hint = 1;

    // Ensure that the session system is initialized
    ret = sessions_bootstrap();
    REQUIRE(ret == VACCEL_OK);

    float a[] = { 1.2, 3.2, 3.0, 4.1, 5.7 };
    float b[] = { 1.1, 0.2, 6.1, 4.6, 5.2 };

    float c[] = { 0.1, 0.1, 0.1, 0.1, 0.1 };
    size_t len_c = sizeof(c) / sizeof(c[0]);

    SECTION("null session")
    {
        REQUIRE(vaccel_fpga_mmult(NULL, a, b, c, len_c) == VACCEL_EINVAL);
    }

    SECTION("valid session and inputs")
    {
        ret = vaccel_sess_init(&sess, 0);
        REQUIRE(ret == VACCEL_OK);
        REQUIRE(sess.session_id);
        REQUIRE(sess.hint == 0);
        REQUIRE(sess.resources);
        REQUIRE(sess.priv == nullptr);

        ret = vaccel_fpga_mmult(&sess, a, b, c, len_c);
        float C_expected[] = { 9.1, 9.1, 9.1, 9.1, 9.1 };

        REQUIRE(vaccel_sess_free(&sess) == VACCEL_OK);
        REQUIRE(ret != VACCEL_ENOTSUP);

        REQUIRE(std::equal(c, c + len_c, C_expected));
    }

    ret = sessions_cleanup();
    REQUIRE(ret == VACCEL_OK);
}
