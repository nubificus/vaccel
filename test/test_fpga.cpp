#include <catch2/catch_test_macros.hpp>

#include <atomic>
using atomic_int = std::atomic<int>;
using atomic_uint = std::atomic<unsigned int>;

extern "C"{
#include "error.h"
#include "fpga.h"
#include "session.h"
}

#include <iostream>

TEST_CASE("vaccel_fpga_add", "[vaccel_fpga_plugins]")
{
    struct vaccel_session sess;
    float A[] = {1, 2, 3, 4, 5};
    float B[] = {1, 2, 6, 4, 5};
    float C[] = {1, 2, 6, 4, 5};

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
        REQUIRE(vaccel_fpga_vadd(NULL, A, B, C, len_A, len_B) ==  VACCEL_EINVAL);
    }

    SECTION("valid session and inputs")
    {
        int ret = vaccel_sess_init(&sess, 0);
        REQUIRE(ret == VACCEL_OK);
        ret = vaccel_fpga_vadd(&sess, A, B, C, len_A, len_B);
        float C_expected[] = {2,4,9,8,10};
        size_t len_C = sizeof(C) / sizeof(C[0]);
        size_t len_C_expected = sizeof(C_expected) / sizeof(C_expected[0]);

        REQUIRE(vaccel_sess_free(&sess) == VACCEL_OK);
        REQUIRE(ret != VACCEL_ENOTSUP);

        REQUIRE(len_C == len_C_expected);
        
        REQUIRE(std::equal(C, C + len_C, C_expected));
    }

}

TEST_CASE("vaccel_fpga_copy", "[vaccel_fpga_plugins]")
{
    struct vaccel_session sess;
    int A[] = {1, 2, 3, 4, 5};
    int B[] = {1, 1, 1, 1, 1};

    size_t len_A = sizeof(A) / sizeof(A[0]);
    size_t len_B = sizeof(B) / sizeof(B[0]);
    
    SECTION("null session")
    {
        REQUIRE(vaccel_fpga_arraycopy(NULL, A, B, len_A) ==  VACCEL_EINVAL);
    }

    SECTION("valid session and inputs")
    {
        int ret = vaccel_sess_init(&sess, 0);
        REQUIRE(ret == VACCEL_OK);
        ret = vaccel_fpga_arraycopy(&sess, A, B, len_A);
        float B_expected[] = {1,2,3,4,5};


        REQUIRE(vaccel_sess_free(&sess) == VACCEL_OK);
        REQUIRE(ret != VACCEL_ENOTSUP);
        
        REQUIRE(std::equal(B, B + len_B, B_expected));
    }

}

TEST_CASE("vaccel_fpga_mmult", "[vaccel_fpga_plugins]")
{
    struct vaccel_session test_sess;
    float a[] = {1.2, 3.2, 3.0, 4.1, 5.7};
    float b[] = {1.1, 0.2 , 6.1, 4.6, 5.2};

    
    
    float c[] = {0.1, 0.1, 0.1, 0.1, 0.1};
    size_t len_c = sizeof(c) / sizeof(c[0]);
    
    SECTION("null session")
    {
        REQUIRE(vaccel_fpga_mmult(NULL, a, b, c, len_c) ==  VACCEL_EINVAL);
    }

    SECTION("valid session and inputs")
    {
        int ret = vaccel_sess_init(&test_sess, 0);
        REQUIRE(ret == VACCEL_OK);
        ret = vaccel_fpga_mmult(&test_sess, a, b, c, len_c);
        float C_expected[] = {9.1, 9.1, 9.1, 9.1, 9.1};

        REQUIRE(vaccel_sess_free(&test_sess) == VACCEL_OK);
        REQUIRE(ret != VACCEL_ENOTSUP);
        
        REQUIRE(std::equal(c, c + len_c, C_expected));
    }

}

TEST_CASE("vaccel_fpga_parallel", "[vaccel_fpga_plugins]")
{
    struct vaccel_session test_sess;
    float a[] = {1.2, 3.2, 3.0, 4.1, 5.7};
    float b[] = {1.1, 0.2, 6.1, 4.6, 5.2};
    size_t len_a = sizeof(a) / sizeof(a[0]);
    float add_out[len_a];
    float mult_out[len_a];
    
    SECTION("null session")
    {
        REQUIRE(vaccel_fpga_parallel(NULL, a, b, add_out, mult_out, len_a) == VACCEL_EINVAL);
    }

    SECTION("valid session and inputs")
    {
        int ret = vaccel_sess_init(&test_sess, 0);
        REQUIRE(ret == VACCEL_OK);
        ret = vaccel_fpga_parallel(&test_sess, a, b, add_out, mult_out, len_a);
        
        // float expected_add_out[] = {2.3, 3.4, 9.1, 8.7, 10.9};
        // float expected_mult_out[] = {1, 1, 1, 1, 1};

        REQUIRE(vaccel_sess_free(&test_sess) == VACCEL_OK);
        REQUIRE(ret != VACCEL_ENOTSUP);

        // REQUIRE(std::equal(add_out, add_out + len_a, expected_add_out));
        // REQUIRE(std::equal(mult_out, mult_out + len_a, expected_mult_out));

    }

}
