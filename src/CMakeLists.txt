# Core runtime sources
set(runtime_src plugin.c vaccel.c session.c log.c)
file(GLOB ops_src ops/*.c)

# core runtime headers
set(headers plugin.h vaccel.h)
file(GLOB ops_headers ops/*.h)

add_library(vaccel SHARED ${headers} ${ops_headers} ${runtime_src} ${ops_src})
target_compile_options(vaccel PUBLIC -Wall -Wextra -Werror -pthread)
target_include_directories(vaccel PRIVATE ${CMAKE_SOURCE_DIR}/src)
set_property(TARGET vaccel PROPERTY LINK_FLAGS "-pthread")
target_link_libraries(vaccel slog)

# Setup make install
install(TARGETS vaccel DESTINATION "${lib_path}")
install(FILES vaccel.h common.h DESTINATION "${include_path}")
install(FILES ${ops_headers} DESTINATION "${include_path}/ops")

# Create the pkg-config file
set(DEST_DIR "${CMAKE_INSTALL_PREFIX}")
CONFIGURE_FILE("vaccel.pc.in" "vaccel.pc" @ONLY)

# Install the vaccel.pc file
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/vaccel.pc DESTINATION "${CMAKE_INSTALL_PREFIX}/share")
