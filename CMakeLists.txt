cmake_minimum_required(VERSION 3.10)

project(vaccelrt
	VERSION 0.1
	DESCRIPTION "VaccelRT library"
	LANGUAGES C CXX)

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
add_subdirectory(src/plugins)
add_subdirectory(examples)

# Create the pkg-config file
set(DEST_DIR "${CMAKE_INSTALL_PREFIX}")
CONFIGURE_FILE("vaccel.pc.in" "vaccel.pc" @ONLY)

# Install the vaccel.pc file
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/vaccel.pc DESTINATION "${CMAKE_INSTALL_PREFIX}/share")
