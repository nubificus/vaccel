cmake_minimum_required(VERSION 3.10)

project(vaccelrt
	VERSION 0.1
	DESCRIPTION "VaccelRT library"
	LANGUAGES C CXX)

# Configuration time options
option(BUILD_PLUGIN_VIRTIO "Build the VirtIO plugin" OFF)
option(BUILD_PLUGIN_JETSON "Build the Jetson plugin" OFF)
option(BUILD_PLUGIN_VSOCK "Build thev vsock plugin" OFF)
option(BUILD_PLUGIN_NOOP "Build the no-op debugging plugin" OFF)
option(BUILD_EXAMPLES "Build the examples" OFF)

## Download GoogleTest framework
configure_file(
	CMakeLists.txt.in
	googletest-download/CMakeLists.txt
)
execute_process(
	COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
	WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/googletest-download
)
execute_process(
	COMMAND ${CMAKE_COMMAND} --build .
	WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/googletest-download
)

# Add GTest directly to our build. This adds the targets:
# gtest, gtest_main, gmock, and gmock_main
add_subdirectory(
	${CMAKE_BINARY_DIR}/googletest-src
	${CMAKE_BINARY_DIR}/googletest-build
)

SET(CMAKE_EXPORT_COMPILE_COMMANDS ON)
IF(EXISTS "${CMAKE_CURRENT_BINARY_DIR}/compile_commands.json")
  EXECUTE_PROCESS(COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${CMAKE_CURRENT_BINARY_DIR}/compile_commands.json
    ${CMAKE_SOURCE_DIR}/compile_commands.json
  )
ENDIF()

# installation paths
set(bin_path "${CMAKE_INSTALL_PREFIX}/bin")
set(lib_path "${CMAKE_INSTALL_PREFIX}/lib")
set(include_path "${CMAKE_INSTALL_PREFIX}/include")

# Add CMake third-party dependencies to be built
add_subdirectory(third-party)

# Add include directories from third-party libraries
include_directories(
	#slog
	third-party/slog/src
)

add_subdirectory(src)
add_subdirectory(plugins)
add_subdirectory(bindings)

if (BUILD_EXAMPLES)
	add_subdirectory(examples)
endif (BUILD_EXAMPLES)

# tests
enable_testing()
add_subdirectory(test)
