cmake_minimum_required(VERSION 3.15)
project(VACCEL_TEST CXX)

set(COMMON_COMPILE_OPTIONS -Wall -Wextra -g -ggdb --coverage)
add_library(tests-main STATIC main.cpp)
set(MY_INCLUDE_DIRECTORIES
    ../third-party/slog/src
    ../src
    ../src/include
    ./fff
    ./catch2
)
# Function to simplify adding tests
function(add_test_target_core TARGET_NAME SOURCE_FILE)
    add_executable(${TARGET_NAME} ${PROJECT_SOURCE_DIR}/unit/${SOURCE_FILE})
    target_include_directories(${TARGET_NAME}
        PRIVATE
            ../third-party/slog/src
            ../src
            ../src/include
            ./fff
            ./catch2
    )
    target_compile_options(${TARGET_NAME} PRIVATE ${COMMON_COMPILE_OPTIONS})
    target_link_libraries(${TARGET_NAME} PRIVATE vaccel tests-main gcov --coverage)
    target_compile_definitions(${TEST_NAME} PRIVATE CATCH_CONFIG_FAST_COMPILE CATCH_CONFIG_DISABLE_MATCHERS)
    add_test(NAME ${TARGET_NAME} COMMAND ${TARGET_NAME})
endfunction()

function(add_test_target_api TARGET_NAME SOURCE_FILE)
    add_executable(${TARGET_NAME} ${PROJECT_SOURCE_DIR}/op/${SOURCE_FILE})
    target_include_directories(${TARGET_NAME}
        PRIVATE
            ../third-party/slog/src
            ../src
            ../src/include
            ../src/include/ops
            ./fff
            ./catch2
            ../src/resources
    )
    target_compile_options(${TARGET_NAME} PRIVATE ${COMMON_COMPILE_OPTIONS})
    target_link_libraries(${TARGET_NAME} PRIVATE vaccel tests-main gcov --coverage)
    target_compile_definitions(${TEST_NAME} PRIVATE CATCH_CONFIG_FAST_COMPILE CATCH_CONFIG_DISABLE_MATCHERS)
    add_test(NAME ${TARGET_NAME} COMMAND ${TARGET_NAME})
endfunction()

set(TESTS_CORE
    test_misc
    test_misc_mock
    test_id_pool
    test_log
    test_bootstrap_shutdown)

set(TESTS_API
    test_fpga
    test_image
    test_image_generic
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

# Tests with a source file (if needed)

add_executable(test_plugin ${PROJECT_SOURCE_DIR}/unit/test_plugin.cpp ../src/plugin.c)
target_include_directories(test_plugin PRIVATE ${MY_INCLUDE_DIRECTORIES})
target_compile_options(test_plugin PRIVATE ${COMMON_COMPILE_OPTIONS})
target_link_libraries(test_plugin PRIVATE vaccel tests-main gcov --coverage)
target_compile_definitions(test_plugin PRIVATE CATCH_CONFIG_FAST_COMPILE CATCH_CONFIG_DISABLE_MATCHERS)
add_test(NAME test_plugin COMMAND test_plugin)

add_executable(test_resource ${PROJECT_SOURCE_DIR}/unit/test_resource.cpp ../src/resources.c) 
target_include_directories(test_resource PRIVATE ${MY_INCLUDE_DIRECTORIES})
target_compile_options(test_resource PRIVATE ${COMMON_COMPILE_OPTIONS})
target_link_libraries(test_resource PRIVATE vaccel tests-main gcov --coverage)
target_compile_definitions(test_resource PRIVATE CATCH_CONFIG_FAST_COMPILE CATCH_CONFIG_DISABLE_MATCHERS)
add_test(NAME test_resource COMMAND test_resource)

add_executable(test_session ${PROJECT_SOURCE_DIR}/unit/test_session.cpp) 
target_include_directories(test_session PRIVATE ${MY_INCLUDE_DIRECTORIES})
target_compile_options(test_session PRIVATE ${COMMON_COMPILE_OPTIONS})
target_link_libraries(test_session PRIVATE vaccel tests-main gcov --coverage)
target_compile_definitions(test_session PRIVATE CATCH_CONFIG_FAST_COMPILE CATCH_CONFIG_DISABLE_MATCHERS)
add_test(NAME test_session COMMAND test_session)


add_executable(test_vaccel ${PROJECT_SOURCE_DIR}/unit/test_vaccel.cpp ../src/resources.c ../src/session.c) 
target_include_directories(test_vaccel PRIVATE ${MY_INCLUDE_DIRECTORIES})
target_compile_options(test_vaccel PRIVATE ${COMMON_COMPILE_OPTIONS})
target_link_libraries(test_vaccel PRIVATE vaccel tests-main gcov --coverage)
target_compile_definitions(test_vaccel PRIVATE CATCH_CONFIG_FAST_COMPILE CATCH_CONFIG_DISABLE_MATCHERS)
add_test(NAME test_vaccel COMMAND test_vaccel)

# Set properties for specific tests

set_tests_properties(
    test_image test_image_generic test_tf_interference test_tf_model test_tf_saved_model test_exec test_minmax test_noop test_blas test_fpga test_vaccel
    PROPERTIES
    ENVIRONMENT "VACCEL_BACKENDS=${CMAKE_BINARY_DIR}/plugins/noop/libvaccel-noop.so"
    ARGS=--order rand --warn NoAssertions
)

set_tests_properties(
    test_log test_id_pool test_misc_mock test_resource test_session test_plugin test_vaccel test_bootstrap_shutdown
    PROPERTIES
    ENVIRONMENT "VACCEL_DEBUG_LEVEL=4"
    ARGS=--order rand --warn NoAssertions
)


set_tests_properties(
    test_misc
    PROPERTIES
    ENVIRONMENT "VACCEL_BACKENDS=${CMAKE_BINARY_DIR}/plugins/noop/libvaccel-noop.so;VACCEL_DEBUG_LEVEL=4"
    ARGS=--order rand --warn NoAssertions
)

