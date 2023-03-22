# Core runtime sources
file(GLOB_RECURSE sources *.c)

# core runtime headers
file(GLOB_RECURSE headers *.h)

add_library(vaccel SHARED ${headers} ${sources})
target_compile_options(vaccel PUBLIC -Wall -Wextra -Werror -pthread)
target_include_directories(vaccel PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_include_directories(vaccel PRIVATE ${CMAKE_SOURCE_DIR}/src/include)
target_include_directories(vaccel PRIVATE ${CMAKE_BINARY_DIR})

target_include_directories(vaccel PRIVATE ${CMAKE_SOURCE_DIR}/src/include)
set_property(TARGET vaccel PROPERTY LINK_FLAGS "-pthread")
set_property(TARGET vaccel PROPERTY C_STANDARD 11)
target_link_libraries(vaccel slog)

find_package(Git)

add_custom_command(OUTPUT ${CMAKE_SRC_DIR}/src/include/vaccel.h
	COMMAND ${CMAKE_COMMAND}
		-D SRC=${CMAKE_SOURCE_DIR}/src/include/vaccel.h.in
		-D DST=${CMAKE_SOURCE_DIR}/src/include/vaccel.h
		-D GIT_EXECUTABLE=${GIT_EXECUTABLE}
		-P ${CMAKE_SOURCE_DIR}/cmake/GenerateGitVersionHeader.cmake
	DEPENDS
		${CMAKE_SOURCE_DIR}/src/include/vaccel.h.in
	COMMENT "Configuring vaccelrt_version"
	VERBATIM)

add_custom_command(OUTPUT ${CMAKE_SRC_DIR}/src/vaccel.h
	COMMAND ${CMAKE_COMMAND}
		-D SRC=${CMAKE_SOURCE_DIR}/src/vaccel.h.in
		-D DST=${CMAKE_SOURCE_DIR}/src/vaccel.h
		-D GIT_EXECUTABLE=${GIT_EXECUTABLE}
		-P ${CMAKE_SOURCE_DIR}/cmake/GenerateGitVersionHeader.cmake
	DEPENDS
		${CMAKE_SOURCE_DIR}/src/vaccel.h.in
	COMMENT "Configuring vaccelrt_version"
	VERBATIM)

add_custom_target(version ALL
	DEPENDS ${CMAKE_SRC_DIR}/src/include/vaccel.h ${CMAKE_SRC_DIR}/src/vaccel.h)

add_dependencies(vaccel version)

# Setup make install
install(TARGETS vaccel DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
	PATTERN "*.in" EXCLUDE)

# Create the pkg-config file
set(DEST_DIR "${CMAKE_INSTALL_PREFIX}")
CONFIGURE_FILE("vaccel.pc.in" "vaccel.pc" @ONLY)

# Install the vaccel.pc file
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/vaccel.pc DESTINATION ${CMAKE_INSTALL_DATAROOTDIR})
