Include(FetchContent)

FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        v3.4.0
)

FetchContent_MakeAvailable(Catch2)

file(GLOB HEADER_FILES "${CMAKE_SOURCE_DIR}" )
file(GLOB HEADER_INCLUDE_FILES "${CMAKE_SOURCE_DIR}")


add_executable(tests ${PROJECT_SOURCE_DIR}/test/test.cpp)
target_compile_options(tests PRIVATE -Wall -Wextra -g -ggdb --coverage -lgcov)
target_link_libraries(tests PRIVATE Catch2::Catch2WithMain gcov --coverage)

add_executable(test_plugins ${PROJECT_SOURCE_DIR}/test/test_plugin.cpp)
target_include_directories(test_plugins
    PRIVATE
        ../third-party/slog/src
        ../src
        ../src/include
        ../build
        ../fff
)
target_compile_options(test_plugins PRIVATE -Wall -Wextra -g -ggdb --coverage -lgcov)
target_link_libraries(test_plugins PRIVATE Catch2::Catch2WithMain vaccel gcov --coverage)

add_executable(test_fpga ${PROJECT_SOURCE_DIR}/test/test_fpga.cpp)
target_include_directories(test_fpga
    PRIVATE
        ../third-party/slog/src
        ../src
        ../src/include
        ../src/include/ops
        ../build
        ../fff
)
target_compile_options(test_fpga PRIVATE -Wall -Wextra -g -ggdb --coverage -lgcov)
target_link_libraries(test_fpga PRIVATE Catch2::Catch2WithMain vaccel vaccel-noop gcov --coverage)

add_executable(test_session ${PROJECT_SOURCE_DIR}/test/test_session.cpp)
target_include_directories(test_session
    PRIVATE
        ../third-party/slog/src    
        ../src
        ../src/include
        ../build
        ../fff
)
target_compile_options(test_session PRIVATE -Wall -Wextra -g -ggdb --coverage -lgcov)
target_link_libraries(test_session PRIVATE Catch2::Catch2WithMain vaccel gcov --coverage)

add_executable(test_misc ${PROJECT_SOURCE_DIR}/test/test_misc.cpp)
target_include_directories(test_misc
    PRIVATE
        ../third-party/slog/src
        ../src/include
        ../src
        ../build
        ../fff
)
target_compile_options(test_misc PRIVATE -Wall -Wextra -g -ggdb --coverage -lgcov)
target_link_libraries(test_misc PRIVATE Catch2::Catch2WithMain vaccel gcov --coverage)


add_executable(
	test_resource
	test_resource.cpp
)
target_include_directories(test_resource
    PRIVATE
        ../src
        ../src/include
        ../fff
)
target_compile_options(test_resource PRIVATE -Wall -Wextra -g -ggdb --coverage -lgcov) 
target_link_libraries(test_resource PRIVATE Catch2::Catch2WithMain vaccel gcov --coverage)

add_executable(test_id_pool ${PROJECT_SOURCE_DIR}/test/test_id_pool.cpp)
target_include_directories(test_id_pool
    PRIVATE
        ../third-party/slog/src
        ../src
        ../src/include
        ../build
        ../fff
)
target_compile_options(test_id_pool PRIVATE -Wall -Wextra -g -ggdb --coverage -lgcov)
target_link_libraries(test_id_pool PRIVATE Catch2::Catch2WithMain vaccel gcov --coverage)

add_executable(test_vaccel ${PROJECT_SOURCE_DIR}/test/test_vaccel.cpp)
target_include_directories(test_vaccel
    PRIVATE
        ../third-party/slog/src
        ../src
        ../src/include
        ../build
        ../fff
)
target_compile_options(test_vaccel PRIVATE -Wall -Wextra -g -ggdb --coverage -lgcov)
target_link_libraries(test_vaccel PRIVATE Catch2::Catch2WithMain vaccel gcov --coverage)


add_executable(test_log ${PROJECT_SOURCE_DIR}/test/test_log.cpp)
target_include_directories(test_log
    PRIVATE
        ../third-party/slog/src
        ../src
        ../src/include
        ../src/include/ops
        ../build
        ../fff
)
target_compile_options(test_log PRIVATE -Wall -Wextra -g -ggdb --coverage -lgcov)
target_link_libraries(test_log PRIVATE Catch2::Catch2WithMain vaccel vaccel-noop gcov --coverage)

add_executable(test_classify ${PROJECT_SOURCE_DIR}/test/test_classify.cpp)
target_include_directories(test_classify
    PRIVATE
        ../third-party/slog/src
        ../src
        ../src/include
        ../src/include/ops
        ../build
        ../fff
)
target_compile_options(test_classify PRIVATE -Wall -Wextra -g -ggdb --coverage -lgcov)
target_link_libraries(test_classify PRIVATE Catch2::Catch2WithMain vaccel vaccel-noop gcov --coverage)

add_executable(test_tf_interference ${PROJECT_SOURCE_DIR}/test/test_tf_interference.cpp)
target_include_directories(test_tf_interference
    PRIVATE
        ../third-party/slog/src
        ../src
        ../src/include
        ../src/include/ops
        ../build
        ../fff
        ../src/resources
)
target_compile_options(test_tf_interference PRIVATE -Wall -Wextra -g -ggdb --coverage -lgcov)
target_link_libraries(test_tf_interference PRIVATE Catch2::Catch2WithMain vaccel vaccel-noop gcov --coverage)

add_executable(test_tf_model ${PROJECT_SOURCE_DIR}/test/test_tf_model.cpp)
target_include_directories(test_tf_model
    PRIVATE
        ../third-party/slog/src
        ../src
        ../src/include
        ../src/include/ops
        ../build
        ../fff
        ../src/resources
)
target_compile_options(test_tf_model PRIVATE -Wall -Wextra -g -ggdb --coverage -lgcov)
target_link_libraries(test_tf_model PRIVATE Catch2::Catch2WithMain vaccel vaccel-noop gcov --coverage)


add_executable(test_tf_saved_model ${PROJECT_SOURCE_DIR}/test/test_tf_saved_model.cpp)
target_include_directories(test_tf_saved_model
    PRIVATE
        ../third-party/slog/src
        ../src
        ../src/include
        ../src/include/ops
        ../build
        ../fff
        ../src/resources
)
target_compile_options(test_tf_saved_model PRIVATE -Wall -Wextra -g -ggdb --coverage -lgcov)
target_link_libraries(test_tf_saved_model PRIVATE Catch2::Catch2WithMain vaccel vaccel-noop gcov --coverage)


list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/_deps/catch2-/src/extras")
include(CTest)
include(Catch)
enable_testing()

add_test(NAME basic_test COMMAND tests)
add_test(NAME test_log COMMAND test_log)
add_test(NAME test_vaccel COMMAND test_vaccel)
add_test(NAME test_id_pool COMMAND test_id_pool)
add_test(NAME test_misc COMMAND test_misc)
add_test(NAME test_resource COMMAND test_resource)
add_test(NAME test_session COMMAND test_session)
add_test(NAME test_plugins COMMAND test_plugins)
add_test(NAME test_fpga COMMAND test_fpga)
add_test(NAME test_classify COMMAND test_classify)
add_test(NAME test_tf_interference COMMAND test_tf_interference)
add_test(NAME test_tf_model COMMAND test_tf_model)
add_test(NAME test_tf_saved_model COMMAND test_tf_saved_model)

set_tests_properties(
    test_vaccel test_fpga
    PROPERTIES
    ENVIRONMENT "VACCEL_DEBUG_LEVEL=4;VACCEL_BACKENDS=${CMAKE_BINARY_DIR}/plugins/noop/libvaccel-noop.so"
    ARGS=--order rand --warn NoAssertions
)

set_tests_properties(
    test_plugins test_classify test_tf_interference test_tf_model test_tf_saved_model
    PROPERTIES
    ENVIRONMENT "VACCEL_BACKENDS=${CMAKE_BINARY_DIR}/plugins/noop/libvaccel-noop.so"
    ARGS=--order rand --warn NoAssertions
)

set_tests_properties(
    basic_test test_log test_id_pool test_misc test_resource test_session
    PROPERTIES
    ARGS=--order rand --warn NoAssertions
)


# catch_discover_tests(tests)
# catch_discover_tests(test_log)
# catch_discover_tests(test_id_pool)
# catch_discover_tests(test_misc)
# catch_discover_tests(test_resource)
# catch_discover_tests(test_session)

# catch_discover_tests(test_plugins PROPERTIES ENVIRONMENT VACCEL_BACKENDS=${CMAKE_BINARY_DIR}/plugins/noop/libvaccel-noop.so)
# catch_discover_tests(test_fpga PROPERTIES ENVIRONMENT VACCEL_BACKENDS=${CMAKE_BINARY_DIR}/plugins/noop/libvaccel-noop.so)
# catch_discover_tests(test_vaccel PROPERTIES ENVIRONMENT VACCEL_BACKENDS=${CMAKE_BINARY_DIR}/plugins/noop/libvaccel-noop.so)

