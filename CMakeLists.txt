cmake_minimum_required(VERSION 3.10)

project(vaccelrt
	VERSION 0.1
	DESCRIPTION "VaccelRT library"
	LANGUAGES C CXX)

# Configuration time options
option(BUILD_PLUGIN_VIRTIO "Build the VirtIO plugin" OFF)
option(BUILD_PLUGIN_JETSON "Build the Jetson plugin" OFF)
option(BUILD_PLUGIN_NOOP "Build the no-op debugging plugin" OFF)
option(BUILD_EXAMPLES "Build the examples" OFF)

SET(CMAKE_EXPORT_COMPILE_COMMANDS ON)
IF(EXISTS "${CMAKE_CURRENT_BINARY_DIR}/compile_commands.json")
  EXECUTE_PROCESS(COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${CMAKE_CURRENT_BINARY_DIR}/compile_commands.json
    ${CMAKE_SOURCE_DIR}/compile_commands.json
  )
ENDIF()

# installation paths
set(lib_path "${CMAKE_INSTALL_PREFIX}/lib")
set(include_path "${CMAKE_INSTALL_PREFIX}/include")

add_subdirectory(src)
add_subdirectory(plugins)

if (BUILD_EXAMPLES)
	add_subdirectory(examples)
endif (BUILD_EXAMPLES)
