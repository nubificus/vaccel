cmake_minimum_required(VERSION 3.15)
project(VACCEL_TEST CXX)
set(CMAKE_CXX_STANDARD 11)

include(FetchContent)
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.4.0
)
FetchContent_MakeAvailable(Catch2)

set(COMMON_COMPILE_OPTIONS -Wall -Wextra -g -ggdb --coverage -lgcov)

# Function to simplify adding tests
function(add_test_target_core TARGET_NAME SOURCE_FILE)
    add_executable(${TARGET_NAME} ${PROJECT_SOURCE_DIR}/${SOURCE_FILE})
    target_include_directories(${TARGET_NAME}
        PRIVATE
            ../third-party/slog/src
            ../src
            ../src/include
            ./fff
    )
    target_compile_options(${TARGET_NAME} PRIVATE ${COMMON_COMPILE_OPTIONS})
    target_link_libraries(${TARGET_NAME} PRIVATE Catch2::Catch2WithMain vaccel gcov --coverage)
    add_test(NAME ${TARGET_NAME} COMMAND ${TARGET_NAME})
endfunction()

function(add_test_target_api TARGET_NAME SOURCE_FILE)
    add_executable(${TARGET_NAME} ${PROJECT_SOURCE_DIR}/${SOURCE_FILE})
    target_include_directories(${TARGET_NAME}
        PRIVATE
            ../third-party/slog/src
            ../src
            ../src/include
            ../src/include/ops
            ./fff
            ../src/resources
    )
    target_compile_options(${TARGET_NAME} PRIVATE ${COMMON_COMPILE_OPTIONS})
    target_link_libraries(${TARGET_NAME} PRIVATE Catch2::Catch2WithMain vaccel gcov --coverage)
    add_test(NAME ${TARGET_NAME} COMMAND ${TARGET_NAME})
endfunction()

set(TESTS_CORE
    test_example
    test_plugin
    test_session
    test_misc
    test_resource
    test_id_pool
    test_vaccel
    test_log
)

set(TESTS_API
    test_fpga
    test_image
    test_tf_interference
    test_tf_model
    test_tf_saved_model
    test_exec
    test_minmax
    test_noop
    test_blas
)

foreach(TEST_NAME ${TESTS_CORE})
    add_test_target_core(${TEST_NAME} ${TEST_NAME}.cpp)
endforeach()

foreach(TEST_NAME ${TESTS_API})
    add_test_target_api(${TEST_NAME} ${TEST_NAME}.cpp)
endforeach()

# Set properties for specific tests

set_tests_properties(
    test_vaccel 
    PROPERTIES
    ENVIRONMENT "VACCEL_DEBUG_LEVEL=4;VACCEL_BACKENDS=${CMAKE_BINARY_DIR}/plugins/noop/libvaccel-noop.so"
    ARGS=--order rand --warn NoAssertions
)

set_tests_properties(
    test_image test_tf_interference test_tf_model test_tf_saved_model test_exec test_minmax test_noop test_blas test_fpga
    PROPERTIES
    ENVIRONMENT "VACCEL_BACKENDS=${CMAKE_BINARY_DIR}/plugins/noop/libvaccel-noop.so"
    ARGS=--order rand --warn NoAssertions
)

set_tests_properties(
    test_example test_log test_id_pool test_misc test_resource test_session test_plugin
    PROPERTIES
    ARGS=--order rand --warn NoAssertions
)