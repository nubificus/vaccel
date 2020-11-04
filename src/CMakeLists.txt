# Core runtime sources
set(runtime_src backend.c vaccel.c vaccel_ops.c  session.c)

# core runtime headers
set(headers backend.h vaccel.h vaccel_ops.h)

add_library(vaccel SHARED ${headers} ${runtime_src})
target_compile_options(vaccel PUBLIC -Wall -Wextra -Werror -pthread)
set_property(TARGET vaccel PROPERTY LINK_FLAGS "-pthread")

# Setup make install
install(TARGETS vaccel DESTINATION "${lib_path}")
install(FILES vaccel.h vaccel_ops.h common.h DESTINATION "${include_path}")

# Create the pkg-config file
set(DEST_DIR "${CMAKE_INSTALL_PREFIX}")
CONFIGURE_FILE("vaccel.pc.in" "vaccel.pc" @ONLY)

# Install the vaccel.pc file
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/vaccel.pc DESTINATION "${CMAKE_INSTALL_PREFIX}/share")
